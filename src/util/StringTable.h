/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "HashTable.h"
#include "String.h"

//
// A collection of interned Strings - it stores their character data, and deduplicates them.
//
// Currently it's just a really thin wrapper around a HashTable, which stores entries as (theString -> theString).
// But, that works and is fairly simple so I'm going with it.
//

struct StringTable {
    HashTable<String> table;
};

void initStringTable(StringTable* table);

String intern(StringTable* table, String input);
