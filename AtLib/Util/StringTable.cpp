/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringTable.h"

String StringTable::intern(StringView input)
{
    if (auto existing = m_table.find(input.deprecated_to_string()); existing.has_value())
        return existing.value();

    auto interned_string = m_arena.allocate_string(input);
    m_table.put(interned_string);
    return interned_string;
}
