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

u64 getCurrentUnixTimestamp()
{
    FILETIME currentFileTime;
    GetSystemTimeAsFileTime(&currentFileTime);

    u64 currentTime = currentFileTime.dwLowDateTime | ((u64)currentFileTime.dwHighDateTime << 32);

    // "File Time" measures 100nanosecond increments, so we divide to get a number of seconds
    u64 seconds = (currentTime / 10000000);

    // File Time counts from January 1, 1601 so we need to subtract the difference to align it with the unix epoch

    u64 unixTime = seconds - 11644473600;

    return unixTime;
}

DateTime platform_getLocalTimeFromTimestamp(u64 unixTimestamp)
{
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
                append(&stb, '\\');
            }
            append(&stb, *it);
            // Trim off a trailing null that might be there.
            if (stb.buffer[stb.length - 1] == '\0')
                stb.length--;
        }
    }

    if (appendWildcard) {
        append(&stb, "\\*"_s);
    }

    append(&stb, '\0');

    String result = getString(&stb);

    ASSERT(isNullTerminated(result)); // Path strings must be null-terminated!

    return result;
}

 void fillFileInfo(WIN32_FIND_DATA* findFileData, FileInfo* result)
{
    result->filename = pushString(&temp_arena(), findFileData->cFileName);
    u64 fileSize = ((u64)findFileData->nFileSizeHigh << 32) + findFileData->nFileSizeLow;
    result->size = (smm)fileSize; // NB: Theoretically it could be more than s64Max, but that seems unlikely?

    result->flags = 0;
    if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        result->flags |= FileFlags::Directory;
    if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        result->flags |= FileFlags::Hidden;
    if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        result->flags |= FileFlags::ReadOnly;
}

DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo* result)
{
    DirectoryListingHandle handle = {};
    WIN32_FIND_DATA findFileData = {};

    handle.path = path;

    auto searchPath = constructPath({ path }, true);

    handle.windows.hFile = FindFirstFile(searchPath.chars, &findFileData);

    if (handle.windows.hFile == INVALID_HANDLE_VALUE) {
        handle.isValid = false;
        u32 errorCode = (u32)GetLastError();
        handle.errorCode = errorCode;
        logError("Failed to read directory listing in \"{0}\". (Error {1})"_s, { handle.path, formatInt(errorCode) });
    } else {
        handle.isValid = true;
        fillFileInfo(&findFileData, result);
    }

    return handle;
}

bool platform_nextFileInDirectory(DirectoryListingHandle* handle, FileInfo* result)
{
    WIN32_FIND_DATA findFileData = {};
    bool succeeded = FindNextFile(handle->windows.hFile, &findFileData) != 0;

    if (succeeded) {
        fillFileInfo(&findFileData, result);
    } else {
        stopDirectoryListing(handle);
    }

    return succeeded;
}

void platform_stopDirectoryListing(DirectoryListingHandle* handle)
{
    FindClose(handle->windows.hFile);
    handle->windows.hFile = INVALID_HANDLE_VALUE;
}

DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path)
{
    DirectoryChangeWatchingHandle handle = {};
    handle.path = path;

    handle.windows.handle = FindFirstChangeNotification(path.chars, true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);

    if (handle.windows.handle == INVALID_HANDLE_VALUE) {
        handle.isValid = false;
        u32 errorCode = (u32)GetLastError();
        handle.errorCode = errorCode;
        logError("Failed to set notification for file changes in \"{0}\". (Error {1})"_s, { handle.path, formatInt(errorCode) });
    } else {
        handle.isValid = true;
    }

    return handle;
}

bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle* handle)
{
    bool filesChanged = false;

    DWORD waitResult = WaitForSingleObject(handle->windows.handle, 0);
    switch (waitResult) {
    case WAIT_FAILED: {
        // Something broke
        handle->isValid = false;
        u32 errorCode = (u32)GetLastError();
        handle->errorCode = errorCode;
        logError("Failed to poll for file changes in \"{0}\". (Error {1})"_s, { handle->path, formatInt(errorCode) });
    } break;

    case WAIT_TIMEOUT: {
        // Nothing to report
        filesChanged = false;
    } break;

    case WAIT_ABANDONED: {
        // Something mutex-related, I think we can ignore this?
        // https://docs.microsoft.com/en-gb/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject
    } break;

    case WAIT_OBJECT_0: {
        // We got a result!
        filesChanged = true;

        // Re-set the notification
        if (FindNextChangeNotification(handle->windows.handle) == false) {
            // something broke
            handle->isValid = false;
            u32 errorCode = (u32)GetLastError();
            handle->errorCode = errorCode;
            logError("Failed to re-set notification for file changes in \"{0}\". (Error {1})"_s, { handle->path, formatInt(errorCode) });
        }
    } break;
    }

    return filesChanged;
}

void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle* handle)
{
    FindCloseChangeNotification(handle->windows.handle);
    handle->windows.handle = INVALID_HANDLE_VALUE;
}
