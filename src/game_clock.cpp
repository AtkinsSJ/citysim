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
	clock->cosmeticDate = makeFakeDateTime(clock->currentDay, clock->timeWithinDay);
}

void incrementClock(GameClock *clock, f32 deltaTime)
{
	clock->timeWithinDay += (deltaTime / SECONDS_PER_GAME_DAY);
	while (clock->timeWithinDay >= 1.0f)
	{
		// Next day!
		clock->currentDay++;
		clock->timeWithinDay -= 1.0f;
	}
	updateCosmeticDate(clock);
}
