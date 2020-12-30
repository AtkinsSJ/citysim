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
