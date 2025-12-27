/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "platform.h"
#define NOMINMAX
#include <shellapi.h>
#include <windows.h>

void openUrlUnsafe(char const* url)
{
    ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
    StringBuilder stb = newStringBuilder(256);

    if (parts.size() > 0) {
        for (auto it = parts.begin(); it != parts.end(); it++) {
            if (it != parts.begin() && (stb.buffer[stb.length - 1] != '\\')) {
                stb.append('\\');
            }
            stb.append(*it);
            // Trim off a trailing null that might be there.
            if (stb.buffer[stb.length - 1] == '\0')
                stb.length--;
        }
    }

    if (appendWildcard) {
        stb.append("\\*"_s);
    }

    stb.append('\0');

    String result = getString(&stb);

    ASSERT(isNullTerminated(result)); // Path strings must be null-terminated!

    return result;
}
