#pragma once

template<typename T>
void expandHashTable(HashTable<T> *table, smm newCapacity)
{
	ASSERT(newCapacity > 0, "Attempted to resize a hash table to {0}", {formatInt(newCapacity)});
	ASSERT(newCapacity > table->capacity, "Attempted to shrink a hash table from {0} to {1}", {formatInt(table->capacity), formatInt(newCapacity)});

	smm oldCount = table->count;
	smm oldCapacity = table->capacity;
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
void initHashTable(HashTable<T> *table, f32 maxLoadFactor=0.75f, smm initialCapacity=0)
{
	*table = {};

	table->maxLoadFactor = maxLoadFactor;

	if (initialCapacity > 0)
	{
		expandHashTable(table, initialCapacity);
	}
}

template<typename T>
HashTableEntry<T> *findEntry(HashTable<T> *table, String key)
{
	u32 index = hashString(key) % table->capacity;
	HashTableEntry<T> *result = null;

	// "Linear probing" - on collision, just keep going until you find an empty slot
	while (true)
	{
		HashTableEntry<T> *entry = table->entries + index;

		if (entry->isOccupied == false || equals(key, entry->key))
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
	if (table->entries == null) return null;

	HashTableEntry<T> *entry = findEntry(table, key);
	if (!entry->isOccupied)
	{
		return null;
	}
	else
	{
		return &entry->value;
	}
}

inline smm growHashTableCapacity(smm capacity)
{
	return (capacity < 8) ? 8 : capacity * 2;
}

template<typename T>
void put(HashTable<T> *table, String key, T value)
{
	// Expand if necessary
	if (table->count + 1 > (table->capacity * table->maxLoadFactor))
	{
		expandHashTable(table, growHashTableCapacity(table->capacity));
	}

	HashTableEntry<T> *entry = findEntry(table, key);

	if (!entry->isOccupied)
	{
		table->count++;
		entry->isOccupied = true;
		entry->key = key;
		entry->keyHash = hashString(key);
	}

	entry->value = value;
}