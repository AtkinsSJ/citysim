/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Lexer.h"

#include <Util/Characters.h>

Lexer::Lexer(StringView input)
    : m_input(input)
{
}

bool Lexer::has_next() const
{
    return m_position < m_input.length();
}

Optional<char> Lexer::peek() const
{
    if (!has_next())
        return {};

    return m_input[m_position];
}

Optional<char> Lexer::consume()
{
    if (!has_next())
        return {};

    return m_input[m_position++];
}

bool Lexer::consume_specific(char const expected)
{
    if (peek() == expected) {
        m_position++;
        return true;
    }

    return false;
}

bool Lexer::consume_specific(StringView const expected)
{
    // Early-out if the expected string is too long to fit.
    if (m_position + expected.length() > m_input.length())
        return false;

    auto const old_position = m_position;

    for (auto i = 0; i < expected.length(); ++i) {
        if (!consume_specific(expected[i])) {
            m_position = old_position;
            return false;
        }
    }

    return true;
}

Optional<StringView> Lexer::consume_until(char const end)
{
    return consume_until([end](char const c) { return c == end; });
}

Optional<StringView> Lexer::consume_until(Function<bool(char)> const& callback)
{
    auto const start_position = m_position;
    auto next = peek();
    while (next.has_value() && !callback(next.value())) {
        m_position++;
        next = peek();
    }

    // Nothing consumed
    if (start_position == m_position)
        return {};

    return m_input.substring(start_position, m_position - start_position);
}

Optional<StringView> Lexer::consume_while(Function<bool(char)> const& callback)
{
    return consume_until([&callback](char const c) { return !callback(c); });
}

void Lexer::discard_whitespace()
{
    (void)consume_while([](char const c) { return is_ascii_whitespace(c); });
}

Optional<StringView> Lexer::consume_token()
{
    return consume_until([](char const c) { return is_ascii_whitespace(c); });
}

Optional<bool> Lexer::consume_bool()
{
    return consume_with_callback<bool>([](Lexer& lexer) -> Optional<bool> {
        auto const token = lexer.consume_token();
        if (token == "true"_sv)
            return true;
        if (token == "false"_sv)
            return false;
        return {};
    });
}
