#pragma once

struct GameClock
{
	// Internal values
	u32 currentDay;    // Starting at 1st Jan year 1.
					   // u32 gives us over 11 million years, so should be plenty!
	f32 timeWithinDay; // 0 to 1

	// "Cosmetic" values generated from the internal values
	DateTime cosmeticDate;

	// TODO: Keep the current date string cached here? My string system doesn't make that very easy.
};

const f32 SECONDS_PER_GAME_DAY = 5.0f;

void initGameClock(GameClock *clock, u32 currentDay = 0, f32 timeOfDay = 0.0f);

void updateCosmeticDate(GameClock *clock);

void incrementClock(GameClock *clock, f32 deltaTime);
