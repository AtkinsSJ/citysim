#pragma once

void initGameClock(GameClock *clock, GameTimestamp currentDay, f32 timeOfDay)
{
	*clock = {};

	clock->currentDay = currentDay;
	clock->timeWithinDay = clamp01(timeOfDay);

	clock->speed = Speed_Slow;
	clock->isPaused = true;

	updateCosmeticDate(clock);
}

void updateCosmeticDate(GameClock *clock)
{
	clock->cosmeticDate = makeFakeDateTime(clock->currentDay, clock->timeWithinDay);
}

u8 incrementClock(GameClock *clock, f32 deltaTime)
{
	u8 clockEvents = 0;

	if (!clock->isPaused)
	{
		clock->timeWithinDay += (deltaTime * GAME_DAYS_PER_SECOND[clock->speed]);

		if (clock->timeWithinDay >= 1.0f)
		{
			// Next day!
			clock->currentDay++;
			clock->timeWithinDay -= 1.0f;
			clockEvents |= ClockEvent_NewDay;
		}

		// We should never be able to increase by more than one day in a single frame!
		ASSERT(clock->timeWithinDay < 1.0f);

		DateTime oldCosmeticDate = clock->cosmeticDate;
		updateCosmeticDate(clock);

		if (oldCosmeticDate.year != clock->cosmeticDate.year)
		{
			clockEvents |= ClockEvent_NewYear;
		}

		if (oldCosmeticDate.month != clock->cosmeticDate.month)
		{
			clockEvents |= ClockEvent_NewMonth;
		}

		if ((clockEvents & ClockEvent_NewDay)
			&& (clock->cosmeticDate.dayOfWeek == Day_Monday))
		{
			clockEvents |= ClockEvent_NewWeek;
		}
	}

	return clockEvents;
}
