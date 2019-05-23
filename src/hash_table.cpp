#pragma once

template<typename T>
void expandHashTable(HashTable<T> *table, s32 newCapacity)
{
	DEBUG_FUNCTION();
	ASSERT(newCapacity > 0, "Attempted to resize a hash table to {0}", {formatInt(newCapacity)});
	ASSERT(newCapacity > table->capacity, "Attempted to shrink a hash table from {0} to {1}", {formatInt(table->capacity), formatInt(newCapacity)});

	s32 oldCount = table->count;
	s32 oldCapacity = table->capacity;
	HashTableEntry<T> *oldItems = table->entries;

	table->capacity = newCapacity;
	table->entries = (HashTableEntry<T>*) allocateRaw(newCapacity * sizeof(HashTableEntry<T>));
	table->count = 0;

	if (oldItems != null)
	{
		// Migrate old entries over
		for (s32 i=0; i < oldCapacity; i++)
		{
			HashTableEntry<T> *oldEntry = oldItems + i;

			if (oldEntry->isOccupied)
			{
				put(table, oldEntry->key, oldEntry->value);
			}
		}

		free(oldItems);
	}

	ASSERT(oldCount == table->count, "Hash table item count changed while expanding it! Old: {0}, new: {1}", {formatInt(oldCount), formatInt(table->count)});
}

template<typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor, s32 initialCapacity)
{
	*table = {};

	table->maxLoadFactor = maxLoadFactor;

	if (initialCapacity > 0)
	{
		expandHashTable(table, initialCapacity);
	}
}

template<typename T>
HashTableEntry<T> *findEntryInternal(HashTable<T> *table, String key)
{
	// DEBUG_FUNCTION();

	u32 hash = hashString(&key);
	u32 index = hash % table->capacity;
	HashTableEntry<T> *result = null;

	// "Linear probing" - on collision, just keep going until you find an empty slot
	while (true)
	{
		HashTableEntry<T> *entry = table->entries + index;

		if (entry->isOccupied == false || 
			(hash == hashString(&entry->key) && equals(key, entry->key)))
		{
			result = entry;
			break;
		}

		index = (index + 1) % table->capacity;
	}

	return result;
}

template<typename T>
T *find(HashTable<T> *table, String key)
{
	DEBUG_FUNCTION();
	
	if (table->entries == null) return null;

	HashTableEntry<T> *entry = findEntryInternal(table, key);
	if (!entry->isOccupied)
	{
		return null;
	}
	else
	{
		return &entry->value;
	}
}

template<typename T>
T findValue(HashTable<T> *table, String key)
{
	return *find(table, key);
}

inline s32 growHashTableCapacity(s32 capacity)
{
	return (capacity < 8) ? 8 : capacity * 2;
}

template<typename T>
T *put(HashTable<T> *table, String key, T value)
{
	DEBUG_FUNCTION();
	// Expand if necessary
	if (table->count + 1 > (table->capacity * table->maxLoadFactor))
	{
		expandHashTable(table, growHashTableCapacity(table->capacity));
	}

	HashTableEntry<T> *entry = findEntryInternal(table, key);

	if (!entry->isOccupied)
	{
		table->count++;
		entry->isOccupied = true;
		hashString(&key);
		entry->key = key;
	}

	entry->value = value;

	return &entry->value;
}

template<typename T>
void clear(HashTable<T> *table)
{
	if (table->count > 0)
	{
		table->count = 0;
		if (table->entries != null)
		{
			for (s32 i=0; i < table->capacity; i++)
			{
				table->entries[i].isOccupied = false;
			}
		}
	}
}

template<typename T>
void freeHashTable(HashTable<T> *table)
{
	if (table->entries != null)
	{
		free(table->entries);
		*table = {};
	}
}

template<typename T>
HashTableIterator<T> iterate(HashTable<T> *table)
{
	HashTableIterator<T> iterator = {};

	iterator.hashTable = table;
	iterator.currentIndex = 0;

	// If the table is empty, we can skip some work.
	iterator.isDone = (table->count == 0);

	// If the first entry is unoccupied, we need to skip ahead
	if (!iterator.isDone && !getEntry(iterator)->isOccupied)
	{
		next(&iterator);
	}

	return iterator;
}

template<typename T>
void next(HashTableIterator<T> *iterator)
{
	while (!iterator->isDone)
	{
		iterator->currentIndex++;

		if (iterator->currentIndex >= iterator->hashTable->capacity)
		{
			iterator->isDone = true;
		}
		else
		{
			// Only stop iterating if we find an occupied entry
			if (getEntry(*iterator)->isOccupied)
			{
				break;
			}
		}
	}
}

template<typename T>
HashTableEntry<T> *getEntry(HashTableIterator<T> iterator)
{
	return iterator.hashTable->entries + iterator.currentIndex;
}

template<typename T>
T *get(HashTableIterator<T> iterator)
{
	return &getEntry(iterator)->value;
}
