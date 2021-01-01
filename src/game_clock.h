#pragma once

enum GameClockSpeed
{
	Speed_Slow,
	Speed_Medium,
	Speed_Fast,

	GameClockSpeedCount
};

// Starting at 1st Jan year 1.
// u32 gives us over 11 million years, so should be plenty!
typedef u32 GameTimestamp;

struct GameClock
{
	// Internal values
	GameTimestamp currentDay;
	f32 timeWithinDay; // 0 to 1

	// "Cosmetic" values generated from the internal values
	DateTime cosmeticDate;

	// TODO: @Speed Keep the current date string cached here? My string system doesn't make that very easy.

	GameClockSpeed speed;
	bool isPaused;
};

const f32 GAME_DAYS_PER_SECOND[GameClockSpeedCount] = {
	1.0f / 5.0f, // Slow
	1.0f / 1.0f, // Medium
	3.0f / 1.0f  // Fast
};

void initGameClock(GameClock *clock, s32 year = 1, MonthOfYear month = Month_January, s32 day = 1, f32 timeOfDay = 0.0f);

void updateCosmeticDate(GameClock *clock);

enum ClockEvents
{
	ClockEvent_NewDay   = 1 << 0,
	ClockEvent_NewWeek  = 1 << 1,
	ClockEvent_NewMonth = 1 << 2,
	ClockEvent_NewYear  = 1 << 3,
};
// Returns a set of ClockEvents for events that occur
u8 incrementClock(GameClock *clock, f32 deltaTime);
