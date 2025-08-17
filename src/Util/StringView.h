/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>
#include <Util/StringBase.h>

class StringView : public StringBase {
public:
    StringView() = default;

    StringView(char const* chars, size_t length)
        : StringBase(chars, length)
    {
    }

    StringView(String const&);
};

inline StringView operator""_sv(char const* chars, size_t length)
{
    return StringView { chars, length };
}
