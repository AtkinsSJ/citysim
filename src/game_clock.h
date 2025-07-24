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

EnumMap<GameClockSpeed, f32> const GAME_DAYS_PER_SECOND {
    1.0f / 5.0f, // Slow
    1.0f / 1.0f, // Medium
    3.0f / 1.0f  // Fast
};

// Starting at 1st Jan year 1.
// u32 gives us over 11 million years, so should be plenty!
typedef u32 GameTimestamp;

struct GameClock {
    // Internal values
    GameTimestamp currentDay;
    f32 timeWithinDay; // 0 to 1

    // "Cosmetic" values generated from the internal values
    DateTime cosmeticDate;

    // TODO: @Speed Keep the current date string cached here? My string system doesn't make that very easy.

    GameClockSpeed speed;
    bool isPaused;
};

void initGameClock(GameClock* clock, GameTimestamp date = 0, f32 timeOfDay = 0.0f);

void updateCosmeticDate(GameClock* clock);

enum class ClockEvents : u8 {
    NewDay,
    NewWeek,
    NewMonth,
    NewYear,
    COUNT,
};
// Returns a set of ClockEvents for events that occur
Flags<ClockEvents> incrementClock(GameClock* clock, f32 deltaTime);

GameTimestamp getCurrentTimestamp();
GameTimestamp timestampFromParts(s32 year, MonthOfYear month, s32 day);
DateTime dateTimeFromTimestamp(GameTimestamp timestamp);
