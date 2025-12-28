/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBuilder.h"
#include <Util/Log.h>
#include <Util/Memory.h>

StringBuilder::StringBuilder(size_t initial_size)
    : m_capacity(max(initial_size, 256))
    , m_buffer(temp_arena().allocate_multiple<char>(m_capacity))
{
}

StringBuilder::StringBuilder(Blob buffer)
    : m_capacity(buffer.size())
    , m_buffer(reinterpret_cast<char*>(buffer.writable_data()))
    , m_fixed_buffer(true)
{
}

void StringBuilder::append(StringView string)
{
    auto new_length = m_length + string.length();
    if (new_length > m_capacity)
        ensure_capacity(new_length);

    copyMemory(string.raw_pointer_to_characters(), m_buffer + m_length, string.length());
    m_length += string.length();
}

void StringBuilder::append(char character)
{
    append({ &character, 1 });
}

void StringBuilder::append(char const* chars, size_t length)
{
    append({ chars, length });
}

void StringBuilder::ensure_capacity(size_t new_capacity)
{
    ASSERT(!m_fixed_buffer);

    logWarn("Expanding StringBuilder"_s);

    s32 target_capacity = max(new_capacity, m_capacity * 2);

    char* newBuffer = temp_arena().allocate_multiple<char>(target_capacity);
    copyMemory(m_buffer, newBuffer, m_length);

    m_buffer = newBuffer;
    m_capacity = target_capacity;
}

void StringBuilder::remove(size_t count)
{
    ASSERT(m_length > count);
    m_length -= count;
}

char StringBuilder::char_at(size_t index) const
{
    ASSERT(index < m_length);
    return m_buffer[index];
}

StringView StringBuilder::to_string_view() const
{
    return StringView { m_buffer, m_length };
}

String StringBuilder::deprecated_to_string() const
{
    return String { m_buffer, m_length };
}
