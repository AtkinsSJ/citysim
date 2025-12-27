/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBase.h"
#include <Util/Memory.h>
#include <Util/MemoryArena.h>
#include <Util/StringView.h>
#include <Util/Unicode.h>

char StringBase::char_at(size_t index) const
{
    ASSERT(index < m_length);
    return m_chars[index];
}

bool StringBase::operator==(StringBase const& other) const
{
    return length() == other.length() && isMemoryEqual(m_chars, other.m_chars, length());
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

bool StringBase::starts_with(char prefix) const
{
    return m_length >= 1 && m_chars[0] == prefix;
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

bool StringBase::ends_with(char suffix) const
{
    return m_length >= 1 && m_chars[m_length - 1] == suffix;
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

Optional<s64> StringBase::to_int() const
{
    if (is_empty())
        return {};

    s64 value = 0;
    s32 startPosition = 0;
    bool isNegative = false;
    if (char_at(0) == '-') {
        isNegative = true;
        startPosition++;
    } else if (char_at(0) == '+') {
        // allow a leading + in case people want it for some reason.
        startPosition++;
    }

    for (s32 position = startPosition; position < length(); position++) {
        value *= 10;

        char c = char_at(position);
        if (c >= '0' && c <= '9') {
            value += c - '0';
        } else {
            return {};
        }
    }

    if (isNegative)
        return -value;
    return value;
}

Optional<double> StringBase::to_double() const
{
    // TODO: Implement this properly!
    // (c runtime functions atof / strtod don't tell you if they failed, they just return 0 which is a valid value!
    String null_terminated = pushString(&temp_arena(), length() + 1);
    copyString(m_chars, m_length, &null_terminated);
    null_terminated.m_chars[length()] = '\0';

    double double_value = atof(null_terminated.m_chars);
    if (double_value == 0.0) {
        // @Hack: 0.0 is returned by atof() if it fails. So, we see if the input really began with a '0' or not.
        // If it didn't, we assume it failed. If it did, we assume it succeeded.
        // Note that if it failed on a value beginning with a '0' character, (eg "0_0") then it will assume it succeeded...
        // But, I don't know a more reliable way without just parsing the float value ourselves so eh!
        // - Sam, 12/09/2019
        if (char_at(0) == '0')
            return double_value;
        return {};
    }

    return double_value;
}

Optional<float> StringBase::to_float() const
{
    if (auto maybe_double = to_double(); maybe_double.has_value())
        return static_cast<float>(maybe_double.value());
    return {};
}

Optional<bool> StringBase::to_bool() const
{
    if (*this == "true"_sv)
        return true;
    if (*this == "false"_sv)
        return false;
    return {};
}
