#pragma once

struct GameClock
{
	// Internal values
	u32 currentDay;    // Starting at 1st Jan year 1.
					   // u32 gives us over 11 million years, so should be plenty!
	f32 timeWithinDay; // 0 to 1

	// "Cosmetic" values generated from the internal values
	u8  cosmeticDay;    // 1 = The 1st of the month
	u8  cosmeticMonth;  // 1 = January
	u32 cosmeticYear;   // 1 = The first year. u32 is probably overkill, but u16 feels possible to reach

	// Keep the current date string cached here? My string system doesn't make that very easy.
};

const u32 DAYS_PER_YEAR = 365;
const u32 MONTHS_PER_YEAR = 12;
const u32 DAYS_PER_MONTH[] = {
//   J   F   M   A   M   J   J   A   S   O   N   D
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void initGameClock(GameClock *clock, u32 currentDay = 0, f32 timeOfDay = 0.0f);

void updateCosmeticDate(GameClock *clock);
