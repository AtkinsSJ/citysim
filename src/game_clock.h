#pragma once

enum GameClockSpeed
{
	Speed_Low,
	Speed_Medium,
	Speed_Fast,

	GameClockSpeedCount
};

struct GameClock
{
	// Internal values
	u32 currentDay;    // Starting at 1st Jan year 1.
					   // u32 gives us over 11 million years, so should be plenty!
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

void initGameClock(GameClock *clock, u32 currentDay = 0, f32 timeOfDay = 0.0f);

void updateCosmeticDate(GameClock *clock);

void incrementClock(GameClock *clock, f32 deltaTime);
