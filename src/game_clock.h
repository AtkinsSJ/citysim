#pragma once

struct GameClock
{
	// Internal values
	u64 currentDay;    // Starting at 1st Jan year 1
	f32 timeWithinDay; // 0 to 1

	// "Cosmetic" values generated from the internal values
	u8  cosmeticDay;    // 1 = The 1st of the month
	u8  cosmeticMonth;  // 1 = January
	u32 cosmeticYear;   // 1 = The first year. u32 is probably overkill, but u16 feels possible to reach

	// Keep the current date string cached here? My string system doesn't make that very easy.
};

// void initGameClock(GameClock *clock, )
