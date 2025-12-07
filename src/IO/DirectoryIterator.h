/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/File.h>
#include <Util/ErrorOr.h>
#include <Util/Flags.h>

#if OS_LINUX
#    include <dirent.h>
#endif

class DirectoryIterator {
public:
    enum class Flags : u8 {
        Recursive, // FIXME: Not implemented!!!
        COUNT,
    };

    static DirectoryIterator invalid();

    ErrorOr<FileInfo> operator*() const;
    DirectoryIterator& operator++();
    DirectoryIterator operator++(int);
    bool operator==(DirectoryIterator const&) const;
    bool operator!=(DirectoryIterator const&) const;

private:
    friend class DirectoryIteratorWrapper;

    struct Data {
#if OS_LINUX
        DIR* dir { nullptr };
        int dir_fd { 0 };
        dirent* dir_entry { nullptr };
#elif OS_WINDOWS
        HANDLE m_file_handle;
#endif

        bool operator==(Data const&) const = default;
    };

    DirectoryIterator(String path, ::Flags<Flags> flags, Data&& thing)
        : m_path(path)
        , m_flags(flags)
        , m_data(move(thing))
    {
    }

    String m_path;
    ::Flags<Flags> m_flags;
    Data m_data;
    Optional<Error> m_error;
};

class DirectoryIteratorWrapper {
public:
    DirectoryIteratorWrapper(String path, Flags<DirectoryIterator::Flags> flags, DirectoryIterator::Data data)
        : m_begin(DirectoryIterator { path, flags, move(data) })
    {
    }

    DirectoryIterator begin() const { return m_begin; }
    DirectoryIterator end() const { return DirectoryIterator::invalid(); }

private:
    DirectoryIterator m_begin;
};

ErrorOr<DirectoryIteratorWrapper> iterate_directory(String const& path, Flags<DirectoryIterator::Flags> = {});
