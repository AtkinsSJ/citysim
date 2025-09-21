/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Optional.h>
#include <Util/OwnPtr.h>
#include <Util/Platform.h>
#include <Util/String.h>

class DirectoryWatcher {
public:
    // FIXME: ErrorOr
    static OwnPtr<DirectoryWatcher> watch(String path);

    // Movable but not copyable
    DirectoryWatcher(DirectoryWatcher&&) = default;
    DirectoryWatcher(DirectoryWatcher const&) = delete;
    DirectoryWatcher& operator=(DirectoryWatcher const&) = delete;

    ~DirectoryWatcher();

    // FIXME: ErrorOr
    Optional<bool> has_changed() const;

private:
#if OS_LINUX
    explicit DirectoryWatcher(String path, int inotify_fd)
        : m_inotify_fd(inotify_fd)
        , m_path(path)
    {
    }
    int m_inotify_fd;
#elif OS_WINDOWS
    HANDLE m_change_handle;
#endif

    String m_path;
};
