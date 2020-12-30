#pragma once

inline DateTime getLocalTimeFromTimestamp(u64 unixTimestamp)
{
	return platform_getLocalTimeFromTimestamp(unixTimestamp);
}

inline DateTime makeFakeDateTime(u32 daysFromStart, f32 timeWithinDay)
{
	DateTime dateTime = {};

	// We don't currently bother with leap years, so the month lengths are hard-coded
	const u32 DAYS_PER_YEAR = 365;
	const u32 DAYS_PER_MONTH[] = {
	//   J   F   M   A   M   J   J   A   S   O   N   D
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	dateTime.year = (daysFromStart / DAYS_PER_YEAR) + 1;
	u32 dayOfYear = daysFromStart % DAYS_PER_YEAR;
	u8 monthIndex = 0;
	while (dayOfYear >= DAYS_PER_MONTH[monthIndex])
	{
		dayOfYear -= DAYS_PER_MONTH[monthIndex];
		monthIndex++;
	}
	dateTime.month = (MonthOfYear)monthIndex;
	dateTime.dayOfMonth = dayOfYear;

	dateTime.dayOfWeek = (DayOfWeek)(daysFromStart % 7);

	// Time as a percentage
	f32 fractionalHours = clamp01(timeWithinDay) * 24.0f;
	dateTime.hour = floor_s32(fractionalHours);
	f32 fractionalMinutes = fraction_f32(fractionalHours) * 60.0f;
	dateTime.minute = floor_s32(fractionalMinutes);
	f32 fractionalSeconds = fraction_f32(fractionalMinutes) * 60.0f;
	dateTime.second = floor_s32(fractionalSeconds);
	dateTime.millisecond = floor_s32(fraction_f32(fractionalSeconds) * 1000.0f);

	return dateTime;
}

String formatDateTime(DateTime dateTime, DateTimeFormat format)
{
	DEBUG_FUNCTION();
	
	String formatString = nullString;

	switch (format)
	{
		case DateTime_ShortDate: {
			formatString = getText("date_format_short_date"_s);
		} break;

		case DateTime_LongDate: {
			formatString = getText("date_format_long_date"_s);
		} break;

		case DateTime_ShortDateTime: {
			formatString = getText("date_format_short_date_time"_s);
		} break;

		case DateTime_LongDateTime: {
			formatString = getText("date_format_long_date_time"_s);
		} break;

		default: {
			logError("Invalid date format requested! ({0})"_s, {formatInt(format)});
		} break;
	}

	StringBuilder stb = newStringBuilder(formatString.length * 10);

	// @Copypasta from myprintf()!

	s32 startOfSymbol = s32Max;
	bool isReadingSymbol = false;
	for (s32 i=0; i<formatString.length; i++)
	{
		switch (formatString.chars[i])
		{
			case '{': {
				startOfSymbol = i+1;
				isReadingSymbol = true;
			} break;

			case '}': {
				s32 endOfSymbol = i;
				if (isReadingSymbol && endOfSymbol > startOfSymbol)
				{
					String symbol = makeString(formatString.chars + startOfSymbol, (endOfSymbol - startOfSymbol), true);

					if (equals(symbol, "year"_s))
					{
						append(&stb, formatInt(dateTime.year));
					}
					else if (equals(symbol, "month"_s))
					{
						append(&stb, formatInt(dateTime.month));
					}
					else if (equals(symbol, "month2"_s))
					{
						append(&stb, formatInt(dateTime.month, 10, 2));
					}
					else if (equals(symbol, "monthN"_s))
					{
						ASSERT(dateTime.month >= 1 && dateTime.month <= 12);
						String monthName = monthNameStrings[dateTime.month - 1];
						append(&stb, getText(monthName));
					}
					else if (equals(symbol, "day"_s))
					{
						append(&stb, formatInt(dateTime.dayOfMonth));
					}
					else if (equals(symbol, "day2"_s))
					{
						append(&stb, formatInt(dateTime.dayOfMonth, 10, 2));
					}
					else if (equals(symbol, "dayN"_s))
					{
						ASSERT(dateTime.dayOfWeek >= 0 && dateTime.dayOfWeek < DayOfWeekCount);
						String dayName = dayNameStrings[dateTime.dayOfWeek];
						append(&stb, getText(dayName));
					}
					else if (equals(symbol, "hour"_s))
					{
						append(&stb, formatInt(dateTime.hour));
					}
					else if (equals(symbol, "hour2"_s))
					{
						append(&stb, formatInt(dateTime.hour, 10, 2));
					}
					else if (equals(symbol, "12hour"_s))
					{
						append(&stb, formatInt(dateTime.hour % 12));
					}
					else if (equals(symbol, "12hour2"_s))
					{
						append(&stb, formatInt(dateTime.hour % 12, 10, 2));
					}
					else if (equals(symbol, "minute"_s))
					{
						append(&stb, formatInt(dateTime.minute));
					}
					else if (equals(symbol, "minute2"_s))
					{
						append(&stb, formatInt(dateTime.minute, 10, 2));
					}
					else if (equals(symbol, "second"_s))
					{
						append(&stb, formatInt(dateTime.second));
					}
					else if (equals(symbol, "second2"_s))
					{
						append(&stb, formatInt(dateTime.second, 10, 2));
					}
					else if (equals(symbol, "millis"_s))
					{
						append(&stb, formatInt(dateTime.millisecond));
					}
					else if (equals(symbol, "millis4"_s))
					{
						append(&stb, formatInt(dateTime.millisecond, 10, 4));
					}
					else if (equals(symbol, "am"_s))
					{
						bool isPM = (dateTime.hour >= 12);
						append(&stb, getText(isPM ? "date_part_pm"_s : "date_part_am"_s));
					}
					else if (equals(symbol, "AM"_s))
					{
						bool isPM = (dateTime.hour >= 12);
						append(&stb, getText(isPM ? "date_part_pmc"_s : "date_part_amc"_s));
					}
					else
					{
						// If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
						logError("Unrecognised date/time symbol '{0}'"_s, {symbol});
						append(&stb, '{');
						append(&stb, symbol);
						append(&stb, '}');
					}
				}

				isReadingSymbol = false;
				startOfSymbol = s32Max;
			} break;

			default: {
				if (!isReadingSymbol)
				{
					s32 startIndex = i;

					while (((i+1) < formatString.length)
						&& (formatString.chars[i + 1] != '{'))
					{
						i++;
					}

					append(&stb, formatString.chars + startIndex, i + 1 - startIndex);
				}
			}
		}
	}

	String result = getString(&stb);
	return result;
}
