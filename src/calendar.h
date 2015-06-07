#pragma once

// calendar.h

enum CalendarSpeed { // Ticks per frame
	SpeedPaused = 0,
	Speed1 = 1000 / FRAMES_PER_SECOND,
	Speed2 = 2000 / FRAMES_PER_SECOND,
	Speed3 = 5000 / FRAMES_PER_SECOND,
};

struct Calendar {
	int32 day; // 0-indexed
	int32 dayOfWeek; // 0-indexed
	int32 month; // 0-indexed
	int32 year; // Real year

	CalendarSpeed speed; // Ticks per frame
	int32 _dayTicksCounter;
};
const int32 ticksPerDay = 1000; // How big does Calendar.dayCounter have to be to increment the day?
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

void initCalendar(Calendar *calendar) {
	calendar->day = 0;
	calendar->dayOfWeek = 0;
	calendar->month = 0;
	calendar->year = 2000;
	calendar->_dayTicksCounter = 0;
	calendar->speed = SpeedPaused;
}

void getDateString(Calendar *calendar, char *buffer) {
	sprintf(buffer, "%s %d of %s %d",
		dayNames[calendar->dayOfWeek],
		calendar->day + 1,
		monthNames[calendar->month],
		calendar->year
	);
}

/**
 * Add an amount of sub-day time to the calendar, return whether the date changed.
 */
bool incrementCalendar(Calendar *calendar) {
	bool dayChanged = false;
	calendar->_dayTicksCounter += calendar->speed;
	if (calendar->_dayTicksCounter >= ticksPerDay) {
		calendar->_dayTicksCounter -= ticksPerDay;
		calendar->day++;
		dayChanged = true;

		calendar->dayOfWeek = (calendar->dayOfWeek + 1) % 7;

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

	return dayChanged;
}