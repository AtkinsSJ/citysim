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

	DayOfWeekCount,
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

const String dayNameStrings[DayOfWeekCount] = {
	"date_part_monday"_s,
	"date_part_tuesday"_s,
	"date_part_wednesday"_s,
	"date_part_thursday"_s,
	"date_part_friday"_s,
	"date_part_saturday"_s,
	"date_part_sunday"_s,
};

const String monthNameStrings[12] = {
	"date_part_month01"_s,
	"date_part_month02"_s,
	"date_part_month03"_s,
	"date_part_month04"_s,
	"date_part_month05"_s,
	"date_part_month06"_s,
	"date_part_month07"_s,
	"date_part_month08"_s,
	"date_part_month09"_s,
	"date_part_month10"_s,
	"date_part_month11"_s,
	"date_part_month12"_s,
};

DateTime getLocalTimeFromTimestamp(u64 unixTimestamp);

//
// This runs like myprintf(), except with named sections instead of numbered ones.
//
// For example, "{day}/{month2}/{year}" could produce "17/04/1974".
//
// Valid names are:
//   year       Full year #, eg 1999
//   month      Month number, eg 4 for April
//   month2     2-digit month number, eg 04 for April
//   monthN     Month name, eg April
//   day        Day of month as number
//   day2       Day of month as 2-digit number
//   dayN       Name of the day of the week
//   hour       Hour in 24-hour clock
//   hour2      2-digit hour in 24-hour clock
//   12hour     Hour in 12-hour clock
//   12hour2    2-digit hour in 12-hour clock
//   minute     Minutes
//   minute2    2-digit minutes
//   second     Seconds
//   second2    2-digit seconds
//   millis     Milliseconds
//   millis4    4-digit milliseconds
//   am         am/pm lowecase
//   AM         AM/PM uppercase
//
String formatDateTime(DateTime dateTime, DateTimeFormat format);
