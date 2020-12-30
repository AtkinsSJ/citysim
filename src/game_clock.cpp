#pragma once

void initGameClock(GameClock *clock, u32 currentDay, f32 timeOfDay)
{
	*clock = {};

	clock->currentDay = currentDay;
	clock->timeWithinDay = clamp01(timeOfDay);

	updateCosmeticDate(clock);
}

void updateCosmeticDate(GameClock *clock)
{
	// For now, we don't support leap years - every year is identical. This means
	// we can just take (currentDay / or % DAYS_PER_YEAR).

	clock->cosmeticYear = (clock->currentDay / DAYS_PER_YEAR) + 1;

	u32 dayOfYear = clock->currentDay % DAYS_PER_YEAR;
	u8 month = 0;
	while (dayOfYear >= DAYS_PER_MONTH[month])
	{
		dayOfYear -= DAYS_PER_MONTH[month];
		month++;
	}

	clock->cosmeticMonth = (u8)(month + 1);
	clock->cosmeticDay = (u8)(dayOfYear + 1); // All that is left in dayOfYear is the position within the month
}
