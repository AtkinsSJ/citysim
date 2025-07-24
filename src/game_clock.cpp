/*
 * Copyright (c) 2020-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "game_clock.h"
#include "AppState.h"
#include <Util/Maths.h>

// We don't bother with leap years, so the month lengths are hard-coded
u32 const DAYS_PER_YEAR = 365;
EnumMap<MonthOfYear, u8> const DAYS_PER_MONTH {
    //  F   M   A   M   J   J   A   S   O   N   D
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void initGameClock(GameClock* clock, GameTimestamp date, f32 timeOfDay)
{
    *clock = {};

    clock->currentDay = date;
    clock->timeWithinDay = clamp01(timeOfDay);

    clock->speed = GameClockSpeed::Slow;
    clock->isPaused = true;

    updateCosmeticDate(clock);
}

void updateCosmeticDate(GameClock* clock)
{
    DateTime dateTime = dateTimeFromTimestamp(clock->currentDay);

    // Time as a percentage
    f32 fractionalHours = clamp01(clock->timeWithinDay) * 24.0f;
    dateTime.hour = floor_s32(fractionalHours);
    f32 fractionalMinutes = fraction_f32(fractionalHours) * 60.0f;
    dateTime.minute = floor_s32(fractionalMinutes);
    f32 fractionalSeconds = fraction_f32(fractionalMinutes) * 60.0f;
    dateTime.second = floor_s32(fractionalSeconds);
    dateTime.millisecond = floor_s32(fraction_f32(fractionalSeconds) * 1000.0f);

    clock->cosmeticDate = dateTime;
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

u8 incrementClock(GameClock* clock, f32 deltaTime)
{
    u8 clockEvents = 0;

    if (!clock->isPaused) {
        clock->timeWithinDay += (deltaTime * GAME_DAYS_PER_SECOND[clock->speed]);

        if (clock->timeWithinDay >= 1.0f) {
            // Next day!
            clock->currentDay++;
            clock->timeWithinDay -= 1.0f;
            clockEvents |= ClockEvent_NewDay;
        }

        // We should never be able to increase by more than one day in a single frame!
        ASSERT(clock->timeWithinDay < 1.0f);

        DateTime oldCosmeticDate = clock->cosmeticDate;
        updateCosmeticDate(clock);

        if (oldCosmeticDate.year != clock->cosmeticDate.year) {
            clockEvents |= ClockEvent_NewYear;
        }

        if (oldCosmeticDate.month != clock->cosmeticDate.month) {
            clockEvents |= ClockEvent_NewMonth;
        }

        if ((clockEvents & ClockEvent_NewDay)
            && (clock->cosmeticDate.dayOfWeek == DayOfWeek::Monday)) {
            clockEvents |= ClockEvent_NewWeek;
        }
    }

    return clockEvents;
}

GameTimestamp getCurrentTimestamp()
{
    return AppState::the().gameState->gameClock.currentDay;
}
