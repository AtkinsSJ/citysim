/*
 * Copyright (c) 2020-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "GameClock.h"
#include <App/App.h>
#include <Sim/Game.h>
#include <Util/Maths.h>

// We don't bother with leap years, so the month lengths are hard-coded
u32 const DAYS_PER_YEAR = 365;
EnumMap<MonthOfYear, u8> const DAYS_PER_MONTH {
    //  F   M   A   M   J   J   A   S   O   N   D
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

GameClock::GameClock(GameTimestamp date, float time_of_day)
    : m_current_day(date)
    , m_current_day_completion(clamp01(time_of_day))
    , m_speed(GameClockSpeed::Slow)
    , m_is_paused(true)
{
    update_cosmetic_date();
}

void GameClock::update_cosmetic_date()
{
    DateTime date_time = dateTimeFromTimestamp(m_current_day);

    // Time as a percentage
    float fractional_hours = clamp01(m_current_day_completion) * 24.0f;
    date_time.hour = floor_s32(fractional_hours);
    float fractional_minutes = fraction_float(fractional_hours) * 60.0f;
    date_time.minute = floor_s32(fractional_minutes);
    float fractional_seconds = fraction_float(fractional_minutes) * 60.0f;
    date_time.second = floor_s32(fractional_seconds);
    date_time.millisecond = floor_s32(fraction_float(fractional_seconds) * 1000.0f);

    m_cosmetic_date = date_time;
}

GameTimestamp timestampFromParts(s32 year, MonthOfYear month, s32 day)
{
    // NB: Day #0 is 1st January, 0001
    GameTimestamp result = 0;

    // Add days for previous years
    result += (year - 1) * DAYS_PER_YEAR;

    // Add days for previous months this year
    for (auto monthIndex = enum_values<MonthOfYear>(); monthIndex != month; ++monthIndex) {
        result += DAYS_PER_MONTH[*monthIndex];
    }

    // Add day within month
    result += (day - 1);

    return result;
}

DateTime dateTimeFromTimestamp(GameTimestamp timestamp)
{
    DateTime dateTime = {};

    dateTime.year = (timestamp / DAYS_PER_YEAR) + 1;
    u32 dayOfYear = timestamp % DAYS_PER_YEAR;
    u8 monthIndex = 0;
    while (dayOfYear >= DAYS_PER_MONTH[monthIndex]) {
        dayOfYear -= DAYS_PER_MONTH[monthIndex];
        monthIndex++;
    }
    dateTime.month = (MonthOfYear)monthIndex;
    dateTime.dayOfMonth = dayOfYear + 1;

    dateTime.dayOfWeek = (DayOfWeek)(timestamp % 7);

    return dateTime;
}

Flags<ClockEvents> GameClock::increment(float delta_time)
{
    Flags<ClockEvents> clock_events;

    if (!m_is_paused) {
        m_current_day_completion += delta_time * GAME_DAYS_PER_SECOND[m_speed];

        if (m_current_day_completion >= 1.0f) {
            // Next day!
            m_current_day++;
            m_current_day_completion -= 1.0f;
            clock_events.add(ClockEvents::NewDay);
        }

        // We should never be able to increase by more than one day in a single frame!
        ASSERT(m_current_day_completion < 1.0f);

        DateTime old_cosmetic_date = m_cosmetic_date;
        update_cosmetic_date();

        if (old_cosmetic_date.year != m_cosmetic_date.year) {
            clock_events.add(ClockEvents::NewYear);
        }

        if (old_cosmetic_date.month != m_cosmetic_date.month) {
            clock_events.add(ClockEvents::NewMonth);
        }

        if (clock_events.has(ClockEvents::NewDay)
            && m_cosmetic_date.dayOfWeek == DayOfWeek::Monday) {
            clock_events.add(ClockEvents::NewWeek);
        }
    }

    return clock_events;
}

GameTimestamp getCurrentTimestamp()
{
    return App::the().game_state()->city.gameClock.current_day();
}
