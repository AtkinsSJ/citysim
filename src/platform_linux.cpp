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

bool platform_deleteFile(String path)
{
    ASSERT(path.is_null_terminated());

    if (unlink(path.raw_pointer_to_characters()) == 0)
        return true;

    if (errno == ENOENT) {
        logInfo("Unable to delete file `{0}` - path does not exist"_s, { path });
    }

    return false;
}
