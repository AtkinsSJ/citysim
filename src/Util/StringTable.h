/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/HashTable.h>
#include <Util/String.h>

//
// A collection of interned Strings - it stores their character data, and deduplicates them.
//
// Currently it's just a really thin wrapper around a HashTable, which stores entries as (theString -> theString).
// But, that works and is fairly simple so I'm going with it.
//

class StringTable {
public:
    String intern(StringView input);

private:
    HashTable<String> m_table;
};
