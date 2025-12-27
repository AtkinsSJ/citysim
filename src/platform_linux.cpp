/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "platform.h"
#include <IO/File.h>
#include <Util/Log.h>
#include <Util/StringBuilder.h>
#include <ctime>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void openUrlUnsafe(char const* url)
{
    char const* format = "xdg-open '%s'";
    smm totalSize = strlen(format) + strlen(url);
    char* buffer = new char[totalSize];
    snprintf(buffer, sizeof(buffer), format, url);
    system(buffer);
    delete[] buffer;
}

u64 getCurrentUnixTimestamp()
{
    return time(nullptr);
}

DateTime platform_getLocalTimeFromTimestamp(u64 unixTimestamp)
{
    DateTime result = {};
    result.unixTimestamp = unixTimestamp;

    time_t time = unixTimestamp;
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
}

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
    // @Copypasta: This is identical to the win32 version except with '/' instead of '\\'!
    StringBuilder stb;

    if (parts.size() > 0) {
        for (auto it = parts.begin(); it != parts.end(); it++) {
            if (it != parts.begin() && (stb.char_at(stb.length() - 1) != '/')) {
                stb.append('/');
            }
            stb.append(*it);
            // Trim off a trailing null that might be there.
            if (stb.char_at(stb.length() - 1) == '\0')
                stb.remove(1);
        }
    }

    if (appendWildcard) {
        stb.append("/*"_s);
    }

    stb.append('\0');

    String result = stb.deprecated_to_string();

    ASSERT(result.is_null_terminated()); // Path strings must be null-terminated!

    return result;
}

bool platform_createDirectory(String _path)
{
    ASSERT(_path.is_null_terminated());

    if (mkdir(_path.m_chars, S_IRWXU) != 0) {
        int result = errno;
        if (result == EEXIST)
            return true;

        if (result == ENOENT) {
            // Part of the path doesn't exist, so we have to create it, piece by piece
            // We do a similar hack to the win32 version: A duplicate path, which we then swap each
            // `/` with a null byte and then back, to mkdir() one path segment at a time.
            String path = pushString(&temp_arena(), _path);
            char* pos = path.m_chars;
            char* afterEndOfPath = path.m_chars + path.m_length;

            while (pos < afterEndOfPath) {
                // This double loop is actually intentional, it's just... weird.
                while (pos < afterEndOfPath) {
                    if (*pos == '/') {
                        *pos = '\0';
                        break;
                    } else {
                        pos++;
                    }
                }

                logInfo("Attempting to create directory: {0}"_s, { path });

                // Create the path
                if (mkdir(path.m_chars, S_IRWXU) != EEXIST) {
                    logError("Unable to create directory `{0}` - failed to create `{1}`."_s, { _path, path });
                    return false;
                }

                *pos = '/';
                pos++;
                continue;
            }

            return true;
        }
    }

    return false;
}

bool platform_deleteFile(String path)
{
    ASSERT(path.is_null_terminated());

    if (unlink(path.m_chars) == 0)
        return true;

    if (errno == ENOENT) {
        logInfo("Unable to delete file `{0}` - path does not exist"_s, { path });
    }

    return false;
}
