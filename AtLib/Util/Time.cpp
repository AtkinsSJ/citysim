/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Assets/AssetManager.h>
#include <Util/Log.h>
#include <Util/StringBuilder.h>
#include <Util/Time.h>

#if OS_LINUX
#    include <ctime>
#elif OS_WINDOWS
// FIXME: Add includes for windows time conversion here.
#endif

DateTime DateTime::from_unix_timestamp(u64 unix_timestamp)
{
#if OS_LINUX
    DateTime result = {};
    result.unixTimestamp = unix_timestamp;

    time_t time = unix_timestamp;
    struct tm* timeInfo = localtime(&time);

    result.year = timeInfo->tm_year + 1900;
    result.month = (MonthOfYear)(timeInfo->tm_mon);
    result.dayOfMonth = timeInfo->tm_mday;
    result.hour = timeInfo->tm_hour;
    result.minute = timeInfo->tm_min;
    result.second = timeInfo->tm_sec;
    result.millisecond = 0;
    result.dayOfWeek = (DayOfWeek)((timeInfo->tm_wday + 6) % 7); // NB: Sunday is 0 in tm, but is 6 for us.

    return result;
#elif OS_WINDOWS
    DateTime result = {};
    result.unixTimestamp = unixTimestamp;

    // NB: Based on the microsoft code at https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
    FILETIME fileTime = {};
    u64 temp = (unixTimestamp + 11644473600) * 10000000;
    fileTime.dwLowDateTime = (DWORD)temp;
    fileTime.dwHighDateTime = temp >> 32;

    // We convert to a local FILETIME first, because I don't see a way to do that conversion later.
    FILETIME localFileTime = {};
    if (FileTimeToLocalFileTime(&fileTime, &localFileTime) == 0) {
        // Error handling!
        u32 errorCode = (u32)GetLastError();
        logError("Failed to convert filetime ({0}) to local filetime. (Error {1})"_s, { formatInt(temp), formatInt(errorCode) });
        return result;
    }

    // Now we can convert that into a SYSTEMTIME
    SYSTEMTIME localSystemTime;
    if (FileTimeToSystemTime(&localFileTime, &localSystemTime) == 0) {
        // Error handling!
        u32 errorCode = (u32)GetLastError();
        logError("Failed to convert local filetime ({0}-{1}) to local systemtime. (Error {2})"_s, { formatInt((u32)localFileTime.dwLowDateTime), formatInt((u32)localFileTime.dwHighDateTime), formatInt(errorCode) });
        return result;
    }

    // Finally, we can fill-in our own DateTime struct with the data
    result.year = localSystemTime.wYear;
    result.month = (MonthOfYear)(localSystemTime.wMonth - 1); // SYSTEMTIME month starts at 1 for January
    result.dayOfMonth = localSystemTime.wDay;
    result.hour = localSystemTime.wHour;
    result.minute = localSystemTime.wMinute;
    result.second = localSystemTime.wSecond;
    result.millisecond = localSystemTime.wMilliseconds;
    result.dayOfWeek = (DayOfWeek)((localSystemTime.wDayOfWeek + 6) % 7); // NB: Sunday is 0 in SYSTEMTIME, but is 6 for us.

    return result;
#endif
}

String formatDateTime(DateTime dateTime, DateTimeFormat format)
{
    DEBUG_FUNCTION();

    String formatString = {};

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

    StringBuilder stb { formatString.length() * 10 };

    // @Copypasta from myprintf()!

    s32 startOfSymbol = s32Max;
    bool isReadingSymbol = false;
    for (s32 i = 0; i < formatString.length(); i++) {
        switch (formatString[i]) {
        case '{': {
            startOfSymbol = i + 1;
            isReadingSymbol = true;
        } break;

        case '}': {
            s32 endOfSymbol = i;
            if (isReadingSymbol && endOfSymbol > startOfSymbol) {
                String symbol { formatString.raw_pointer_to_characters() + startOfSymbol, (size_t)(endOfSymbol - startOfSymbol), WithHash::Yes };

                if (symbol == "year"_s) {
                    stb.append(formatInt(dateTime.year));
                } else if (symbol == "month"_s) {
                    stb.append(formatInt(to_underlying(dateTime.month) + 1));
                } else if (symbol == "month2"_s) {
                    stb.append(formatInt(to_underlying(dateTime.month) + 1, 10, 2));
                } else if (symbol == "monthN"_s) {
                    ASSERT(to_underlying(dateTime.month) >= 0 && to_underlying(dateTime.month) < to_underlying(MonthOfYear::COUNT));
                    stb.append(getText(month_names[dateTime.month]));
                } else if (symbol == "day"_s) {
                    stb.append(formatInt(dateTime.dayOfMonth));
                } else if (symbol == "day2"_s) {
                    stb.append(formatInt(dateTime.dayOfMonth, 10, 2));
                } else if (symbol == "dayN"_s) {
                    ASSERT(to_underlying(dateTime.dayOfWeek) >= 0 && to_underlying(dateTime.dayOfWeek) < to_underlying(DayOfWeek::COUNT));
                    stb.append(getText(day_names[dateTime.dayOfWeek]));
                } else if (symbol == "hour"_s) {
                    stb.append(formatInt(dateTime.hour));
                } else if (symbol == "hour2"_s) {
                    stb.append(formatInt(dateTime.hour, 10, 2));
                } else if (symbol == "12hour"_s) {
                    stb.append(formatInt(dateTime.hour % 12));
                } else if (symbol == "12hour2"_s) {
                    stb.append(formatInt(dateTime.hour % 12, 10, 2));
                } else if (symbol == "minute"_s) {
                    stb.append(formatInt(dateTime.minute));
                } else if (symbol == "minute2"_s) {
                    stb.append(formatInt(dateTime.minute, 10, 2));
                } else if (symbol == "second"_s) {
                    stb.append(formatInt(dateTime.second));
                } else if (symbol == "second2"_s) {
                    stb.append(formatInt(dateTime.second, 10, 2));
                } else if (symbol == "millis"_s) {
                    stb.append(formatInt(dateTime.millisecond));
                } else if (symbol == "millis3"_s) {
                    stb.append(formatInt(dateTime.millisecond, 10, 3));
                } else if (symbol == "am"_s) {
                    bool isPM = (dateTime.hour >= 12);
                    stb.append(getText(isPM ? "date_part_pm"_s : "date_part_am"_s));
                } else if (symbol == "AM"_s) {
                    bool isPM = (dateTime.hour >= 12);
                    stb.append(getText(isPM ? "date_part_pmc"_s : "date_part_amc"_s));
                } else {
                    // If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
                    logError("Unrecognised date/time symbol '{0}'"_s, { symbol });
                    stb.append('{');
                    stb.append(symbol);
                    stb.append('}');
                }
            }

            isReadingSymbol = false;
            startOfSymbol = s32Max;
        } break;

        default: {
            if (!isReadingSymbol) {
                auto start_index = i;
                auto index_of_next_bracket = formatString.find('{', SearchFrom::Start, start_index);
                if (index_of_next_bracket.has_value()) {
                    stb.append(formatString.view().substring(start_index, index_of_next_bracket.value() - start_index));
                    i = index_of_next_bracket.value() - 1;
                } else {
                    // Not found, so output the rest of the string.
                    stb.append(formatString.view().substring(start_index));
                    i = formatString.length();
                }
            }
        }
        }
    }

    return stb.deprecated_to_string();
}

u32 get_current_unix_timestamp()
{
#if OS_LINUX
    return time(nullptr);
#elif OS_WINDOWS
    FILETIME currentFileTime;
    GetSystemTimeAsFileTime(&currentFileTime);

    u64 currentTime = currentFileTime.dwLowDateTime | ((u64)currentFileTime.dwHighDateTime << 32);

    // "File Time" measures 100nanosecond increments, so we divide to get a number of seconds
    u64 seconds = (currentTime / 10000000);

    // File Time counts from January 1, 1601 so we need to subtract the difference to align it with the unix epoch

    u64 unixTime = seconds - 11644473600;

    return unixTime;
#endif
}
