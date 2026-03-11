/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TokenReader.h"
#include <Util/Unicode.h>

static bool is_split_char(char input, Optional<char> const& split_char)
{
    if (split_char.has_value())
        return input == split_char.value();
    return isWhitespace(input);
}

u32 TokenReader::remaining_token_count(Optional<char> const& split_char) const
{
    u32 result = 0;
    u32 position = 0;
    while (position < m_input.length()) {
        while (position <= m_input.length() && is_split_char(m_input[position], split_char)) {
            position++;
        }

        if (position < m_input.length()) {
            result++;

            // length
            while (position < m_input.length() && !is_split_char(m_input[position], split_char)) {
                position++;
            }
        }
    }

    return result;
}

Optional<StringView> TokenReader::next_token(Optional<char> const& split_char)
{
    auto token = peek_token(split_char);
    if (!token.has_value())
        return {};

    auto remainder_start = token.value().length();
    while (remainder_start < m_input.length() && is_split_char(m_input[remainder_start], split_char))
        ++remainder_start;
    m_input = m_input.substring(remainder_start).with_whitespace_trimmed();

    return token.value().with_whitespace_trimmed();
}

Optional<StringView> TokenReader::peek_token(Optional<char> const& split_char) const
{
    // FIXME: This assumes the input doesn't start with a split-char.
    auto token_length = 0;
    while (token_length < m_input.length() && !is_split_char(m_input[token_length], split_char))
        ++token_length;

    if (token_length == 0)
        return {};

    return m_input.substring(0, token_length);
}
