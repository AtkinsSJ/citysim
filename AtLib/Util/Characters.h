/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

constexpr bool is_ascii_whitespace(char c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
        return true;
    default:
        return false;
    }
}

constexpr bool is_ascii_alphabetic(char const c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr bool is_ascii_numeric(char const c)
{
    return c >= '0' && c <= '9';
}

constexpr bool is_ascii_alphanumeric(char const c)
{
    return is_ascii_alphabetic(c) || is_ascii_numeric(c);
}
