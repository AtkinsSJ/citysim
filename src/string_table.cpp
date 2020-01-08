#pragma once

void initStringTable(StringTable *table)
{
	initHashTable(&table->table);
}

String intern(StringTable *table, String input)
{
	HashTable<String> *hashTable = &table->table;

	HashTableEntry<String> *entry = findEntryInternal(hashTable, input);
	if (!entry->isOccupied)
	{
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
