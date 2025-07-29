/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringTable.h"
#include <Util/Assert.h>

void initStringTable(StringTable* table)
{
    initHashTable(&table->table);
}

String intern(StringTable* table, String input)
{
    ASSERT(stringIsValid(input));

    HashTable<String>* hashTable = &table->table;

    HashTableEntry<String>* entry = hashTable->findOrAddEntry(input);
    if (!entry->isOccupied) {
        hashTable->count++;
        entry->isOccupied = true;
        entry->isGravestone = false;

        String internedString = pushString(&hashTable->keyDataArena, input);
        hashString(&internedString);
        entry->key = internedString;
        entry->value = internedString;
    }

    return entry->value;
}
