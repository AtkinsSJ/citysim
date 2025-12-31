/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Optional.h>

enum class SearchFrom : u8 {
    Start,
    End,
};

enum class TrimSide : u8 {
    Start,
    End,
    Both,
};

class StringBase {
public:
    StringBase() = default;

    char char_at(size_t index) const;
    char operator[](size_t index) const { return char_at(index); }
    char const* raw_pointer_to_characters() const { return m_chars; }

    bool operator==(StringBase const&) const;

    size_t length() const { return m_length; }
    bool is_empty() const { return m_length == 0; }

    Optional<size_t> find(char needle, SearchFrom = SearchFrom::Start, Optional<size_t> start_index = {}) const;
    bool contains(char) const;
    bool starts_with(StringBase const& prefix) const;
    bool starts_with(char prefix) const;
    bool ends_with(StringBase const& suffix) const;
    bool ends_with(char suffix) const;

    StringView view() const;
    operator StringView() const;

    StringView substring(size_t start, Optional<size_t> length = {}) const;
    StringView with_whitespace_trimmed(TrimSide = TrimSide::Both) const;

    Optional<s64> to_int() const;
    Optional<double> to_double() const;
    Optional<float> to_float() const;
    Optional<bool> to_bool() const;

protected:
    StringBase(char const* chars, size_t length)
        : m_chars(chars)
        , m_length(length)
    {
    }

    char const* m_chars;
    size_t m_length;
};
