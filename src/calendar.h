#pragma once

// calendar.h

struct Calendar {
	int32 day; // 0-indexed
	int32 month; // 0-indexed
	int32 year; // Real year

	int32 ticksPerFrame;
	int32 _dayTicksCounter;
};
const int32 ticksPerDay = 2000; // How big does Calendar.dayCounter have to be to increment the day?
const int32 daysInMonth[] = {
	31, // J
	28, // F
	31, // M
	30, // A
	31, // M
	30, // J
	31, // J
	31, // A
	30, // S
	31, // O
	30, // N
	31, // D
};
const char *dayNames[] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday",
};
const char *monthNames[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December,"
};

void initCalendar(Calendar *calendar, int32 framesPerSecond) {
	calendar->day = 0;
	calendar->month = 0;
	calendar->year = 2000;
	calendar->_dayTicksCounter = 0;
	calendar->ticksPerFrame = 1000 / framesPerSecond;
}

void getDateString(Calendar *calendar, char *buffer) {
	sprintf(buffer, "%s %d of %s %d",
		dayNames[calendar->day],
		calendar->day + 1,
		monthNames[calendar->month],
		calendar->year
	);
}

/**
 * Add an amount of sub-day time to the calendar, and return how many days have passed.
 */
int32 incrementCalendar(Calendar *calendar) {
	int32 daysPassed = 0;
	calendar->_dayTicksCounter += calendar->ticksPerFrame;
	while (calendar->_dayTicksCounter >= ticksPerDay) {
		calendar->_dayTicksCounter -= ticksPerDay;
		calendar->day++;
		daysPassed++;

		// Month rollover
		if (calendar->day == daysInMonth[calendar->month]) {
			calendar->day = 0;
			calendar->month++;

			// Year rollover
			if (calendar->month == 12) {
				calendar->month = 0;
				calendar->year++;
			}
		}
	}

	return daysPassed;
}