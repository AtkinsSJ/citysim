#pragma once

// We don't currently bother with leap years, so the month lengths are hard-coded
const u32 DAYS_PER_YEAR = 365;
const u32 DAYS_PER_MONTH[] = {
//   J   F   M   A   M   J   J   A   S   O   N   D
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

void initGameClock(GameClock *clock, s32 year, MonthOfYear month, s32 day, f32 timeOfDay)
{
	*clock = {};

	// NB: Day #0 is 1st January, 0001

	GameTimestamp currentDay = 0;

	// Add days for previous years
	currentDay += (year - 1) * DAYS_PER_YEAR;

	// Add days for previous months this year
	for (s32 monthIndex = Month_January; monthIndex < month; monthIndex++)
	{
		currentDay += DAYS_PER_MONTH[monthIndex];
	}

	// Add day within month
	currentDay += (day - 1);

	clock->currentDay = currentDay;

	clock->timeWithinDay = clamp01(timeOfDay);

	clock->speed = Speed_Slow;
	clock->isPaused = true;

	updateCosmeticDate(clock);

	// Make sure we match our input
	ASSERT(clock->cosmeticDate.year == year);
	ASSERT(clock->cosmeticDate.month == month);
	ASSERT(clock->cosmeticDate.dayOfMonth == day);
}

void updateCosmeticDate(GameClock *clock)
{
	DateTime dateTime = {};

	dateTime.year = (clock->currentDay / DAYS_PER_YEAR) + 1;
	u32 dayOfYear = clock->currentDay % DAYS_PER_YEAR;
	u8 monthIndex = 0;
	while (dayOfYear >= DAYS_PER_MONTH[monthIndex])
	{
		dayOfYear -= DAYS_PER_MONTH[monthIndex];
		monthIndex++;
	}
	dateTime.month = (MonthOfYear)monthIndex;
	dateTime.dayOfMonth = dayOfYear + 1;

	dateTime.dayOfWeek = (DayOfWeek)(clock->currentDay % 7);

	// Time as a percentage
	f32 fractionalHours = clamp01(clock->timeWithinDay) * 24.0f;
	dateTime.hour = floor_s32(fractionalHours);
	f32 fractionalMinutes = fraction_f32(fractionalHours) * 60.0f;
	dateTime.minute = floor_s32(fractionalMinutes);
	f32 fractionalSeconds = fraction_f32(fractionalMinutes) * 60.0f;
	dateTime.second = floor_s32(fractionalSeconds);
	dateTime.millisecond = floor_s32(fraction_f32(fractionalSeconds) * 1000.0f);

	clock->cosmeticDate = dateTime;
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
