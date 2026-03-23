/*
 * Copyright (c) 2020-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/EnumMap.h>
#include <Util/Flags.h>
#include <Util/Time.h>

enum class GameClockSpeed : u8 {
    Slow,
    Medium,
    Fast,

    COUNT
};

EnumMap<GameClockSpeed, float> const GAME_DAYS_PER_SECOND {
    1.0f / 5.0f, // Slow
    1.0f / 1.0f, // Medium
    3.0f / 1.0f  // Fast
};

// Starting at 1st Jan year 1.
// u32 gives us over 11 million years, so should be plenty!
typedef u32 GameTimestamp;

enum class ClockEvents : u8 {
    NewDay,
    NewWeek,
    NewMonth,
    NewYear,
    COUNT,
};

class GameClock {
public:
    GameClock(GameTimestamp date = 0, float time_of_day = 0.0f);

    Flags<ClockEvents> increment(float delta_time);

    GameClockSpeed speed() const { return m_speed; }
    bool is_paused() const { return m_is_paused; }
    void set_speed(GameClockSpeed speed) { m_speed = speed; }
    void set_is_paused(bool paused) { m_is_paused = paused; }

    GameTimestamp current_day() const { return m_current_day; }
    float current_day_completion() const { return m_current_day_completion; }

    DateTime const& cosmetic_date() const { return m_cosmetic_date; }

private:
    void update_cosmetic_date();

    // Internal values
    GameTimestamp m_current_day;
    float m_current_day_completion; // 0 to 1

    // "Cosmetic" values generated from the internal values
    DateTime m_cosmetic_date;

    // TODO: @Speed Keep the current date string cached here? My string system doesn't make that very easy.

    GameClockSpeed m_speed { GameClockSpeed::Slow };
    bool m_is_paused { true };
};

GameTimestamp timestampFromParts(s32 year, MonthOfYear month, s32 day);
DateTime dateTimeFromTimestamp(GameTimestamp timestamp);
