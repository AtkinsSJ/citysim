/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringTable.h"

String StringTable::intern(StringView input)
{
    String input_as_string = input.deprecated_to_string();
    HashTableEntry<String>& entry = *m_table.findOrAddEntry(input_as_string);
    if (!entry.isOccupied) {
        m_table.count++;
        entry.isOccupied = true;
        entry.isGravestone = false;

        String interned_string = m_table.keyDataArena.allocate_string(input);
        interned_string.hash();
        entry.key = interned_string;
        entry.value = interned_string;
    }

    return entry.value;
}
