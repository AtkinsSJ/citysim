#pragma once

enum DayOfWeek
{
	Day_Monday,
	Day_Tuesday,
	Day_Wednesday,
	Day_Thursday,
	Day_Friday,
	Day_Saturday,
	Day_Sunday,
};

struct DateTime
{
	u64 unixTimestamp; // Global time

	s32 year;
	s32 month;
	s32 dayOfMonth;
	s32 hour;
	s32 minute;
	s32 second;
	s32 millisecond;

	DayOfWeek dayOfWeek;
};

enum DateTimeFormat
{
	DateTime_ShortDate,
	DateTime_LongDate,
	DateTime_ShortDateTime,
	DateTime_LongDateTime,
};

DateTime getLocalTimeFromTimestamp(u64 unixTimestamp);

String formatDateTime(DateTime dateTime, DateTimeFormat format);
