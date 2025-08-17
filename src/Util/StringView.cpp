/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringView.h"
#include "String.h"

StringView::StringView(String const& string)
    : StringBase(string.chars, string.length)
{
}
