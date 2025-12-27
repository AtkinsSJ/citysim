/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DirectoryIterator.h"
#include <Util/Deferred.h>
#include <Util/String.h>

#if OS_LINUX
#    include <errno.h>
#    include <sys/stat.h>
#elif OS_WINDOWS
#endif

DirectoryIterator DirectoryIterator::invalid()
{
    return DirectoryIterator { ""_s, {}, {} };
}

ErrorOr<FileInfo> DirectoryIterator::operator*() const
{
    if (!m_data.dir_entry)
        return Error { "No more directory entries to iterate."_s };

    struct stat stat_buffer {};
    FileInfo file_info {
        .filename = String::from_null_terminated(m_data.dir_entry->d_name),
        .flags = {},
        .size = 0,
    };

    if (fstatat(m_data.dir_fd, m_data.dir_entry->d_name, &stat_buffer, 0) != 0) {
        auto error_code = errno;
        auto error_message = myprintf("Failed to stat file \"{0}\". (Error {1})"_s, { file_info.filename, formatInt(error_code) });
        return Error { error_message };
    }

    if (stat_buffer.st_mode & S_IFDIR)
        file_info.flags.add(FileFlags::Directory);
    if (file_info.filename[0] == '.')
        file_info.flags.add(FileFlags::Hidden);
    // TODO: ReadOnly flag, which... we never use!

    file_info.size = stat_buffer.st_size;
    return file_info;
}

static ErrorOr<dirent*> read_dirent(String const& path, DIR* dir)
{
    errno = 0;
    dirent* entry = readdir(dir);
    if (errno != 0) {
        auto error_code = errno;
        auto error_message = myprintf("Failed to read directory entry in \"{0}\". (Error {1})"_s, { path, formatInt(error_code) });
        return Error { error_message };
    }
    return entry;
}

DirectoryIterator& DirectoryIterator::operator++()
{
#if OS_LINUX
    errno = 0;
    dirent* entry = readdir(m_data.dir);
    if (errno != 0) {
        auto error_code = errno;
        auto new_iterator = invalid();
        auto error_message = myprintf("Failed to read directory entry in \"{0}\". (Error {1})"_s, { m_path, formatInt(error_code) });
        new_iterator.m_error = Error { error_message };
        closedir(m_data.dir);
        *this = move(new_iterator);
        return *this;
    }

    if (!entry) {
        closedir(m_data.dir);
        *this = invalid();
        return *this;
    }

    m_data.dir_entry = entry;
    return *this;

#elif OS_WINDOWS
    static_assert("I should really implement this.");
#endif
}

DirectoryIterator DirectoryIterator::operator++(int)
{
    auto result = *this;
    ++(*this);
    return result;
}

bool DirectoryIterator::operator==(DirectoryIterator const& other) const
{
    // I don't think we need to compare anything else.
    return m_data == other.m_data;
}

bool DirectoryIterator::operator!=(DirectoryIterator const& other) const
{
    return !(*this == other);
}

ErrorOr<DirectoryIteratorWrapper> iterate_directory(String const& path, Flags<DirectoryIterator::Flags> flags)
{
    ASSERT(path.is_null_terminated());

#if OS_LINUX
    auto* dir = opendir(path.m_chars);
    Deferred close_dir = [&dir] {
        if (dir)
            closedir(dir);
    };
    if (!dir) {
        auto error_code = errno;
        auto error = myprintf("Failed to read directory listing in \"{0}\". (Error {1})"_s, { path, formatInt(error_code) });
        return Error { error };
    }

    auto dir_fd = dirfd(dir);
    if (dir_fd == -1) {
        auto error_code = errno;
        auto error = myprintf("Failed to read FD of directory \"{0}\". (Error {1})"_s, { path, formatInt(error_code) });
        return Error { error };
    }

    auto maybe_first_entry = read_dirent(path, dir);
    if (maybe_first_entry.is_error())
        return maybe_first_entry.release_error();

    if (!maybe_first_entry.value())
        return DirectoryIteratorWrapper { path, flags, {} };

    close_dir.disable();
    return DirectoryIteratorWrapper { path, flags, { .dir = dir, .dir_fd = dir_fd, .dir_entry = maybe_first_entry.release_value() } };

#elif OS_WINDOWS
    static_assert("I should really implement this.");
#endif
}
