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
    StringBuilder stb = newStringBuilder(256);

    if (parts.size() > 0) {
        for (auto it = parts.begin(); it != parts.end(); it++) {
            if (it != parts.begin() && (stb.buffer[stb.length - 1] != '/')) {
                append(&stb, '/');
            }
            append(&stb, *it);
            // Trim off a trailing null that might be there.
            if (stb.buffer[stb.length - 1] == '\0')
                stb.length--;
        }
    }

    if (appendWildcard) {
        append(&stb, "/*"_s);
    }

    append(&stb, '\0');

    String result = getString(&stb);

    ASSERT(result.is_null_terminated()); // Path strings must be null-terminated!

    return result;
}

bool platform_createDirectory(String _path)
{
    ASSERT(_path.is_null_terminated());

    if (mkdir(_path.chars, S_IRWXU) != 0) {
        int result = errno;
        if (result == EEXIST)
            return true;

        if (result == ENOENT) {
            // Part of the path doesn't exist, so we have to create it, piece by piece
            // We do a similar hack to the win32 version: A duplicate path, which we then swap each
            // `/` with a null byte and then back, to mkdir() one path segment at a time.
            String path = pushString(&temp_arena(), _path);
            char* pos = path.chars;
            char* afterEndOfPath = path.chars + path.length;

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
                if (mkdir(path.chars, S_IRWXU) != EEXIST) {
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

    if (unlink(path.chars) == 0)
        return true;

    if (errno == ENOENT) {
        logInfo("Unable to delete file `{0}` - path does not exist"_s, { path });
    }

    return false;
}

struct FileInfo;

struct DirectoryListingHandle;

static void fillFileInfo(DirectoryListingHandle const& handle, dirent const& entry, FileInfo& result)
{
    struct stat statBuffer {};
    result = {
        .filename = makeString(entry.d_name),
        .flags = {},
        .size = 0,
    };

    if (fstatat(handle.dirFD, entry.d_name, &statBuffer, 0) != 0) {
        logInfo("Failed to stat file \"{0}\". (Error {1})"_s, { result.filename, formatInt(errno) });
        return;
    }

    if (statBuffer.st_mode & S_IFDIR)
        result.flags.add(FileFlags::Directory);
    if (findIndexOfChar(result.filename, '.', false).orDefault(-1) == 0)
        result.flags.add(FileFlags::Hidden);
    // TODO: ReadOnly flag, which... we never use!

    result.size = statBuffer.st_size;
}

static bool readNextDirEntry(DirectoryListingHandle& handle, FileInfo& result)
{
    handle.isValid = true;
    errno = 0;
    dirent* entry = readdir(handle.dir);
    if (errno != 0) {
        handle.isValid = false;
        handle.errorCode = errno;
        logError("Failed to read directory entry in \"{0}\". (Error {1})"_s, { handle.path, formatInt(handle.errorCode) });
        return false;
    }

    if (!entry) {
        stopDirectoryListing(&handle);
        return false;
    }

    fillFileInfo(handle, *entry, result);
    return true;
}

DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo* result)
{
    ASSERT(path.is_null_terminated());

    DirectoryListingHandle handle = {};

    handle.dir = opendir(path.chars);
    if (handle.dir == nullptr) {
        handle.isValid = false;
        handle.errorCode = errno;
        logError("Failed to read directory listing in \"{0}\". (Error {1})"_s, { handle.path, formatInt(handle.errorCode) });
        return handle;
    }
    handle.dirFD = dirfd(handle.dir);
    if (handle.dirFD == -1) {
        handle.isValid = false;
        handle.errorCode = errno;
        logError("Failed to read FD of directory \"{0}\". (Error {1})"_s, { handle.path, formatInt(handle.errorCode) });
        return handle;
    }

    readNextDirEntry(handle, *result);

    return handle;
}

bool platform_nextFileInDirectory(DirectoryListingHandle* handle, FileInfo* result)
{
    return readNextDirEntry(*handle, *result);
}

void platform_stopDirectoryListing(DirectoryListingHandle* handle)
{
    if (handle->dir) {
        closedir(handle->dir);
        handle->dir = nullptr;
    }
}

struct DirectoryChangeWatchingHandle;
DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path)
{
    ASSERT(path.is_null_terminated());

    DirectoryChangeWatchingHandle handle = {};
    handle.path = path;

    // Initialize inotify if needed
    handle.inotify_fd = inotify_init();
    if (handle.inotify_fd < 0) {
        handle.isValid = false;
        handle.errorCode = -errno;
        logError("Failed to initialize inotify. (Error {})"_s, { formatInt(handle.errorCode) });
        return handle;
    }

    handle.watcher_fd = inotify_add_watch(handle.inotify_fd, path.chars, IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (handle.watcher_fd < 0) {
        handle.isValid = false;
        handle.errorCode = -errno;
        logError("Failed to add inotify watch to \"{}\". (Error {})"_s, { path, formatInt(handle.errorCode) });
        return handle;
    }

    handle.isValid = true;

    return handle;
}

bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle* handle)
{
    if (!handle->inotify_fd)
        return false;

    pollfd fd_info {
        .fd = handle->inotify_fd,
        .events = POLLIN,
        .revents = 0,
    };

    auto result = poll(&fd_info, 1, 0);
    if (result == 0) {
        // Nothing happened.
        return false;
    }

    if (result < 0) {
        // Error
        handle->isValid = false;
        handle->errorCode = -errno;
        logError("Failed to poll inotify. (Error {})"_s, { formatInt(handle->errorCode) });
        return handle;
    }

    ASSERT(fd_info.revents & POLLIN);

    // TODO: Note which files changed, which we might want at some point?
    auto constexpr buffer_size = 2048;
    u8 buffer[buffer_size];
    auto read_result = read(handle->inotify_fd, buffer, buffer_size);
    if (read_result < 0) {
        handle->isValid = false;
        handle->errorCode = -errno;
        logError("Failed to read from inotify. (Error {})"_s, { formatInt(handle->errorCode) });
    }

    return true;
}

void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle* handle)
{
    inotify_rm_watch(handle->inotify_fd, handle->watcher_fd);
    handle->watcher_fd = 0;
    close(handle->inotify_fd);
    handle->inotify_fd = 0;
}
