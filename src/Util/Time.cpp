/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../platform.h"
#include <Assets/AssetManager.h>
#include <Debug/Debug.h>
#include <Util/Log.h>
#include <Util/StringBuilder.h>
#include <Util/Time.h>

DateTime getLocalTimeFromTimestamp(u64 unixTimestamp)
{
    return platform_getLocalTimeFromTimestamp(unixTimestamp);
}

String formatDateTime(DateTime dateTime, DateTimeFormat format)
{
    DEBUG_FUNCTION();

    String formatString = nullString;

    switch (format) {
    case DateTimeFormat::ShortDate: {
        formatString = getText("date_format_short_date"_s);
    } break;

    case DateTimeFormat::LongDate: {
        formatString = getText("date_format_long_date"_s);
    } break;

    case DateTimeFormat::ShortDateTime: {
        formatString = getText("date_format_short_date_time"_s);
    } break;

    case DateTimeFormat::LongDateTime: {
        formatString = getText("date_format_long_date_time"_s);
    } break;

    default: {
        logError("Invalid date format requested! ({0})"_s, { formatInt(format) });
    } break;
    }

    StringBuilder stb = newStringBuilder(formatString.length * 10);

    // @Copypasta from myprintf()!

    s32 startOfSymbol = s32Max;
    bool isReadingSymbol = false;
    for (s32 i = 0; i < formatString.length; i++) {
        switch (formatString.chars[i]) {
        case '{': {
            startOfSymbol = i + 1;
            isReadingSymbol = true;
        } break;

        case '}': {
            s32 endOfSymbol = i;
            if (isReadingSymbol && endOfSymbol > startOfSymbol) {
                String symbol = makeString(formatString.chars + startOfSymbol, (endOfSymbol - startOfSymbol), true);

                if (symbol == "year"_s) {
                    append(&stb, formatInt(dateTime.year));
                } else if (symbol == "month"_s) {
                    append(&stb, formatInt(to_underlying(dateTime.month) + 1));
                } else if (symbol == "month2"_s) {
                    append(&stb, formatInt(to_underlying(dateTime.month) + 1, 10, 2));
                } else if (symbol == "monthN"_s) {
                    ASSERT(to_underlying(dateTime.month) >= 0 && to_underlying(dateTime.month) < to_underlying(MonthOfYear::COUNT));
                    append(&stb, getText(month_names[dateTime.month]));
                } else if (symbol == "day"_s) {
                    append(&stb, formatInt(dateTime.dayOfMonth));
                } else if (symbol == "day2"_s) {
                    append(&stb, formatInt(dateTime.dayOfMonth, 10, 2));
                } else if (symbol == "dayN"_s) {
                    ASSERT(to_underlying(dateTime.dayOfWeek) >= 0 && to_underlying(dateTime.dayOfWeek) < to_underlying(DayOfWeek::COUNT));
                    append(&stb, getText(day_names[dateTime.dayOfWeek]));
                } else if (symbol == "hour"_s) {
                    append(&stb, formatInt(dateTime.hour));
                } else if (symbol == "hour2"_s) {
                    append(&stb, formatInt(dateTime.hour, 10, 2));
                } else if (symbol == "12hour"_s) {
                    append(&stb, formatInt(dateTime.hour % 12));
                } else if (symbol == "12hour2"_s) {
                    append(&stb, formatInt(dateTime.hour % 12, 10, 2));
                } else if (symbol == "minute"_s) {
                    append(&stb, formatInt(dateTime.minute));
                } else if (symbol == "minute2"_s) {
                    append(&stb, formatInt(dateTime.minute, 10, 2));
                } else if (symbol == "second"_s) {
                    append(&stb, formatInt(dateTime.second));
                } else if (symbol == "second2"_s) {
                    append(&stb, formatInt(dateTime.second, 10, 2));
                } else if (symbol == "millis"_s) {
                    append(&stb, formatInt(dateTime.millisecond));
                } else if (symbol == "millis3"_s) {
                    append(&stb, formatInt(dateTime.millisecond, 10, 3));
                } else if (symbol == "am"_s) {
                    bool isPM = (dateTime.hour >= 12);
                    append(&stb, getText(isPM ? "date_part_pm"_s : "date_part_am"_s));
                } else if (symbol == "AM"_s) {
                    bool isPM = (dateTime.hour >= 12);
                    append(&stb, getText(isPM ? "date_part_pmc"_s : "date_part_amc"_s));
                } else {
                    // If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
                    logError("Unrecognised date/time symbol '{0}'"_s, { symbol });
                    append(&stb, '{');
                    append(&stb, symbol);
                    append(&stb, '}');
                }
            }

            isReadingSymbol = false;
            startOfSymbol = s32Max;
        } break;

        default: {
            if (!isReadingSymbol) {
                s32 startIndex = i;

                while (((i + 1) < formatString.length)
                    && (formatString.chars[i + 1] != '{')) {
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
