/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/MemoryArena.h>
#include <Util/String.h>

/**
 * This is for temporary string allocation, like in myprintf().
 * As such, when it runs out of room it allocates a new buffer without deallocating the old one -
 * because we're assuming we'll use a temporary memory arena that will be reset once we're done.
 * So, BE CAREFUL!
 * If you want to keep the resulting String around after, make a copy of it somewhere permanent.
 *
 * It's permitted to pass any MemoryArena you like, and it will use that for allocations. So, if
 * you want to keep it around long term, you can use a separate arena and reset that once you're
 * done.
 *
 * TODO: Maybe we should use a chunked approach? That way we're not throwing away the old buffer,
 * but adding an additional one that carries on. That'a a bit more complicated though.
 * For now, I've added logging when we call expand() so we can see if that's ever an issue.
 */

class StringBuilder {
public:
    explicit StringBuilder(size_t initial_size = 256);
    explicit StringBuilder(Blob buffer);

    void append(StringView);
    void append(char);
    void append(char const*, size_t length);

    void ensure_capacity(size_t new_capacity);
    void remove(size_t count);

    char char_at(size_t index) const;

    size_t length() const { return m_length; }
    bool is_empty() const { return m_length == 0; }

    StringView to_string_view() const;
    String deprecated_to_string() const;

private:
    size_t m_capacity;
    size_t m_length { 0 };
    char* m_buffer;
    bool m_fixed_buffer { false };
};
