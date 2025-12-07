/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DirectoryWatcher.h"
#include <Util/ErrorOr.h>
#include <Util/Log.h>

#if OS_LINUX
#    include <cerrno>
#    include <sys/inotify.h>
#    include <sys/poll.h>
#    include <unistd.h>
#endif

ErrorOr<NonnullOwnPtr<DirectoryWatcher>> DirectoryWatcher::watch(String path)
{
    ASSERT(path.is_null_terminated());

#if OS_LINUX
    auto inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        auto error_code = -errno;
        // FIXME: This should make a permanent copy of the string.
        //        For now, temporary is fine as it'll live for the rest of the frame, and we won't persist errors, but that's a footgun.
        auto error = myprintf("Failed to initialize inotify. (Error {})"_s, { formatInt(error_code) });
        close(inotify_fd);
        return Error { error };
    }

    if (inotify_add_watch(inotify_fd, path.chars, IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO) < 0) {
        auto error_code = -errno;
        // FIXME: This should make a permanent copy of the string, see above.
        auto error = myprintf("Failed to add inotify watch to \"{}\". (Error {})"_s, { path, formatInt(error_code) });
        close(inotify_fd);
        return Error { error };
    }

    // FIXME: Recurse!

    return adopt_own(*new DirectoryWatcher { path, inotify_fd });
#elif OS_WINDOWS
    auto handle = FindFirstChangeNotification(path.chars, true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (handle == INVALID_HANDLE_VALUE) {
        auto error_code = GetLastError();
        // FIXME: This should make a permanent copy of the string, see above.
        auto error = myprintf("Failed to set notification for file changes in \"{0}\". (Error {1})"_s, { path, formatInt(error_code) });
        return Error { error };
    }
    return adopt_own(*new DirectoryWatcher { path, handle });
#endif
}

DirectoryWatcher::~DirectoryWatcher()
{
#if OS_LINUX
    close(m_inotify_fd);
#elif OS_WINDOWS
    FindCloseChangeNotification(m_change_handle);
#endif
}

ErrorOr<bool> DirectoryWatcher::has_changed() const
{
#if OS_LINUX
    pollfd fd_info {
        .fd = m_inotify_fd,
        .events = POLLIN,
        .revents = 0,
    };

    auto result = poll(&fd_info, 1, 0);
    if (result == 0) {
        // Nothing happened.
        return false;
    }

    if (result < 0) {
        auto error_code = -errno;
        // FIXME: This should make a permanent copy of the string.
        //        For now, temporary is fine as it'll live for the rest of the frame, and we won't persist errors, but that's a footgun.
        auto error = myprintf("Failed to poll inotify for \"{}\". (Error {})"_s, { m_path, formatInt(error_code) });
        return Error { error };
    }

    ASSERT(fd_info.revents & POLLIN);

    // TODO: Note which files changed, which we might want at some point?
    auto constexpr buffer_size = 2048;
    u8 buffer[buffer_size];
    auto read_result = read(m_inotify_fd, buffer, buffer_size);
    if (read_result < 0) {
        auto error_code = -errno;
        // FIXME: This should make a permanent copy of the string, see above.
        auto error = myprintf("Failed to read from inotify for \"{}\". (Error {})"_s, { m_path, formatInt(error_code) });
        return Error { error };
    }

    // FIXME: Update recursive watchers to respond to newly added/removed directories.

    return true;

#elif OS_WINDOWS
    DWORD waitResult = WaitForSingleObject(m_handle, 0);
    switch (waitResult) {
    case WAIT_FAILED: {
        // Something broke
        auto error_code = GetLastError();
        // FIXME: This should make a permanent copy of the string, see above.
        auto error = myprintf("Failed to poll for file changes in \"{0}\". (Error {1})"_s, { m_path, formatInt(error_code) });
        return Error { error };
    }

    case WAIT_TIMEOUT: {
        // Nothing to report
        return false;
    }

    case WAIT_ABANDONED: {
        // Something mutex-related, I think we can ignore this?
        // https://docs.microsoft.com/en-gb/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject
        return false;
    }

    case WAIT_OBJECT_0: {
        // We got a result!

        // Re-set the notification
        if (FindNextChangeNotification(m_handle) == false) {
            // something broke
            auto error_code = GetLastError();
            // FIXME: This should make a permanent copy of the string, see above.
            auto error = myprintf("Failed to re-set notification for file changes in \"{0}\". (Error {1})"_s, { m_path, formatInt(error_code) });
            return Error { error };
        }

        return true;
    }
    }

    VERIFY_NOT_REACHED();
#endif
}
