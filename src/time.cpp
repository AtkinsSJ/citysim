#pragma once

inline DateTime getLocalTimeFromTimestamp(u64 unixTimestamp)
{
	return platform_getLocalTimeFromTimestamp(unixTimestamp);
}

String formatDateTime(DateTime dateTime, DateTimeFormat format)
{
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
					// TODO: accelerate this into a single append()
					char c = formatString.chars[i];
					append(&stb, c);
				}
			}
		}
	}

	String result = getString(&stb);
	return result;
}
