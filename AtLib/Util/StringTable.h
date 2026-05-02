/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/HashSet.h>
#include <Util/MemoryArena.h>
#include <Util/String.h>

//
// A collection of interned Strings - it stores their character data, and deduplicates them.
//

class StringTable {
public:
    String intern(StringView input);

private:
    HashSet<String> m_table;
    MemoryArena m_arena { "string_table"_s };
};
