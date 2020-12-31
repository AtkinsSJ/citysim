#pragma once

void initGameClock(GameClock *clock, GameTimestamp currentDay, f32 timeOfDay)
{
	*clock = {};

	clock->currentDay = currentDay;
	clock->timeWithinDay = clamp01(timeOfDay);

	clock->speed = Speed_Slow;
	clock->isPaused = false;

	updateCosmeticDate(clock);
}

void updateCosmeticDate(GameClock *clock)
{
	clock->cosmeticDate = makeFakeDateTime(clock->currentDay, clock->timeWithinDay);
}

void incrementClock(GameClock *clock, f32 deltaTime)
{
	if (!clock->isPaused)
	{
		clock->timeWithinDay += (deltaTime * GAME_DAYS_PER_SECOND[clock->speed]);
		while (clock->timeWithinDay >= 1.0f)
		{
			// Next day!
			clock->currentDay++;
			clock->timeWithinDay -= 1.0f;
		}
		updateCosmeticDate(clock);
	}
}
