/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/StringView.h>

class TokenReader {
public:
    explicit TokenReader(StringView input)
        : m_input(input)
    {
    }

    u32 remaining_token_count(Optional<char> const& split_char = {}) const;
    Optional<StringView> next_token(Optional<char> const& split_char = {});
    Optional<StringView> peek_token(Optional<char> const& split_char = {}) const;
    StringView remaining_input() const { return m_input; }

private:
    StringView m_input;
};
