/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBase.h"
#include <Util/Memory.h>
#include <Util/StringView.h>
#include <Util/Unicode.h>

char StringBase::char_at(size_t index) const
{
    ASSERT(index < m_length);
    return m_chars[index];
}

Optional<size_t> StringBase::find(char needle, SearchFrom search_direction, Optional<size_t> start_index) const
{
    if (is_empty())
        return {};

    switch (search_direction) {
    case SearchFrom::Start: {
        auto index = start_index.value_or(0);
        while (index < m_length) {
            if (m_chars[index] == needle)
                return index;
            index++;
        }
        break;
    }
    case SearchFrom::End: {
        auto index = start_index.value_or(m_length - 1);
        while (true) {
            if (m_chars[index] == needle)
                return index;
            if (index == 0)
                return {};
            index--;
        }
        break;
    }
    }

    return {};
}

bool StringBase::contains(char c) const
{
    return find(c).has_value();
}

bool StringBase::starts_with(StringBase const& prefix) const
{
    bool result = false;

    if (m_length == prefix.m_length) {
        result = *this == prefix;
    } else if (m_length > prefix.m_length) {
        result = isMemoryEqual<char>(prefix.m_chars, m_chars, prefix.m_length);
    } else {
        // Otherwise, prefix > s, so it can't end with it!
        result = false;
    }

    return result;
}

bool StringBase::ends_with(StringBase const& suffix) const
{
    bool result = false;

    if (m_length == suffix.m_length) {
        result = *this == suffix;
    } else if (m_length > suffix.m_length) {
        result = isMemoryEqual<char>(suffix.m_chars, &m_chars[m_length - suffix.m_length], suffix.m_length);
    } else {
        // Otherwise, suffix > s, so it can't end with it!
        result = false;
    }

    return result;
}

StringView StringBase::substring(size_t start, Optional<size_t> length) const
{
    auto substring_length = min(length.value_or(m_length), m_length - start);
    return StringView { m_chars + start, substring_length };
}

StringView StringBase::with_whitespace_trimmed(TrimSide trim_side) const
{
    size_t start = 0;
    size_t length = m_length;

    if (trim_side != TrimSide::End) {
        while (length > 0 && isWhitespace(m_chars[start], false)) {
            ++start;
            --length;
        }
    }

    if (trim_side != TrimSide::Start) {
        while (length > 0 && isWhitespace(m_chars[start + length - 1], false)) {
            --length;
        }
    }

    return substring(start, length);
}
