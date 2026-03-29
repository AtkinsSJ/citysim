/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBuilder.h"
#include <Util/Log.h>
#include <Util/Memory.h>
#include <Util/MemoryArena.h>

StringBuilder::StringBuilder(size_t initial_size)
    : m_buffer(temp_arena().allocate_multiple<char>(max(initial_size, 256)))
{
}

StringBuilder::StringBuilder(Blob buffer)
    : m_buffer { buffer.size(), reinterpret_cast<char*>(buffer.writable_data()) }
    , m_fixed_buffer(true)
{
}

void StringBuilder::append(StringBase const& string)
{
    auto new_length = m_length + string.length();
    if (new_length > m_buffer.size())
        ensure_capacity(new_length);

    copyMemory(string.raw_pointer_to_characters(), m_buffer.raw_data() + m_length, string.length());
    m_length += string.length();
}

void StringBuilder::append(char character)
{
    append(StringView { &character, 1 });
}

void StringBuilder::append(char const* chars, size_t length)
{
    append(StringView { chars, length });
}

void StringBuilder::ensure_capacity(size_t new_capacity)
{
    ASSERT(!m_fixed_buffer);

    logWarn("Expanding StringBuilder"_s);

    s32 target_capacity = max(new_capacity, m_buffer.size() * 2);

    auto newBuffer = temp_arena().allocate_multiple<char>(target_capacity);
    copyMemory(m_buffer.raw_data(), newBuffer.raw_data(), m_length);
    m_buffer = newBuffer;
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
    return StringView { m_buffer.raw_data(), m_length };
}

String StringBuilder::deprecated_to_string() const
{
    return String { m_buffer.raw_data(), m_length };
}
