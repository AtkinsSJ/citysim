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

bool platform_createDirectory(String _path)
{
    bool succeeded = true;

    ASSERT(isNullTerminated(_path));

    // First, we can skip a lot of work if the directory already exists, or all but the
    // last path segment do.
    if (CreateDirectory(_path.chars, nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
        // Nothing left to do!
    } else {
        // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createdirectorya
        // We're only allowed to create the final section of the path.
        // So, we start from the left and call CreateDirectory() for each section.
        // If it fails, but the error is ERROR_ALREADY_EXISTS, that's fine.

        // This is something of a hack...
        // Rather than have to allocate a bunch of temporary strings, we can just allocate
        // one, and then replace each '\' character with a null to terminate that string,
        // then switch it back for the next one.
        // The path is guaranteed to be null terminated at the end.

        String path = pushString(&temp_arena(), _path);

        char* pos = path.chars;
        char* afterEndOfPath = path.chars + path.length;

        while (pos < afterEndOfPath) {
            // This double loop is actually intentional, it's just... weird.
            while (pos < afterEndOfPath) {
                if (*pos == '\\') {
                    *pos = '\0';
                    break;
                } else {
                    pos++;
                }
            }

            logInfo("Attempting to create directory: {0}"_s, { path });

            // Create the path
            if (!CreateDirectory(path.chars, nullptr)) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    succeeded = false;
                    logError("Unable to create directory `{0}` - failed to create `{1}`."_s, { _path, path });
                    break;
                }
            }

            *pos = '\\';
            pos++;
            continue;
        }
    }

    return succeeded;
}

bool platform_deleteFile(String path)
{
    bool succeeded = true;

    if (DeleteFile(path.chars) == 0) {
        succeeded = false;
    }

    return succeeded;
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
