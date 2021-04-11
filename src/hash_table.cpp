#pragma once

template <typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor, s32 initialCapacity)
{
	*table = {};

	ASSERT(maxLoadFactor < 1.0f);
	table->maxLoadFactor = maxLoadFactor;

	if (initialCapacity > 0)
	{
		table->expand(ceil_s32(initialCapacity / maxLoadFactor));
	}

	initMemoryArena(&table->keyDataArena, KB(4), KB(4));
}

template<typename T>
void freeHashTable(HashTable<T> *table)
{
	ASSERT(!table->hasFixedMemory);

	if (table->entries != null)
	{
		deallocateRaw(table->entries);
		*table = {};
	}
}

template <typename T>
smm calculateHashTableDataSize(s32 capacity, f32 maxLoadFactor)
{
	s32 desiredCapacity = ceil_s32((f32)capacity / maxLoadFactor);

	return desiredCapacity * sizeof(HashTableEntry<T>);
}

template <typename T>
void initFixedSizeHashTable(HashTable<T> *table, s32 capacity, f32 maxLoadFactor, Blob entryData)
{
	ASSERT(maxLoadFactor < 1.0f);

	*table = {};
	table->maxLoadFactor = maxLoadFactor;
	table->capacity = ceil_s32((f32)capacity / maxLoadFactor);
	table->hasFixedMemory = true;

	smm requiredSize = calculateHashTableDataSize<T>(capacity, maxLoadFactor);
	ASSERT(requiredSize >= entryData.size);

	table->entries = (HashTableEntry<T> *)entryData.memory;

	// TODO: Eliminate this somehow
	initMemoryArena(&table->keyDataArena, KB(4), KB(4));
}

template <typename T>
HashTable<T> allocateFixedSizeHashTable(MemoryArena *arena, s32 capacity, f32 maxLoadFactor)
{
	HashTable<T> result;

	smm hashTableDataSize = calculateHashTableDataSize<T>(capacity, maxLoadFactor);
	Blob hashTableData = allocateBlob(arena, hashTableDataSize);
	initFixedSizeHashTable<T>(&result, capacity, maxLoadFactor, hashTableData);

	return result;
}

template <typename T>
inline bool HashTable<T>::isInitialised()
{
	return (entries != null || maxLoadFactor > 0.0f);
}

template<typename T>
bool HashTable<T>::contains(String key)
{
	if (entries == null) return false;

	HashTableEntry<T> *entry = findEntry(key);
	if (!entry->isOccupied)
	{
		return false;
	}
	else
	{
		return true;
	}
}

template<typename T>
HashTableEntry<T> *HashTable<T>::findEntry(String key)
{
	ASSERT(isInitialised());

	u32 hash = hashString(&key);
	u32 index = hash % capacity;
	HashTableEntry<T> *result = null;

	// "Linear probing" - on collision, just keep going until you find an empty slot
	s32 itemsChecked = 0;
	while (true)
	{
		HashTableEntry<T> *entry = entries + index;

		if (entry->isGravestone)
		{
			// Store the first gravestone we find, in case we fail to find the "real" option
			if (result == null) result = entry;
		}
		else if ((entry->isOccupied == false) || 
			(hash == hashString(&entry->key) && equals(key, entry->key)))
		{
			// If the entry is unoccupied, we'd rather re-use the gravestone we found above
			if (entry->isOccupied || result == null)
			{
				result = entry;
			}
			break;
		}

		index = (index + 1) % capacity;

		// Prevent the edge case infinite loop if all unoccupied spaces are gravestones
		itemsChecked++;
		if (itemsChecked >= capacity) break;
	}

	return result;
}

template<typename T>
HashTableEntry<T> *HashTable<T>::findOrAddEntry(String key)
{
	ASSERT(isInitialised());

	HashTableEntry<T> *result = null;

	if (capacity > 0)
	{
		result = findEntry(key);

		if (!result->isOccupied)
		{
			// Expand if needed!
			if (count + 1 > (capacity * maxLoadFactor))
			{
				ASSERT(!hasFixedMemory);

				s32 newCapacity = max(
					ceil_s32((count + 1) / maxLoadFactor),
					(capacity < 8) ? 8 : capacity * 2
				);
				expand(newCapacity);

				// We now have to search again, because the result we got before is now invalid
				result = findEntry(key);
			}
		}
	}
	else
	{
		// We're at 0 capacity, so expand
		s32 newCapacity = max(
			ceil_s32((count + 1) / maxLoadFactor),
			(capacity < 8) ? 8 : capacity * 2
		);
		expand(newCapacity);

		// We now have to search again, because the result we got before is now invalid
		result = findEntry(key);
	}

	return result;
}

template<typename T>
Maybe<T*> HashTable<T>::find(String key)
{
	Maybe<T*> result = makeFailure<T*>();

	if (entries != null)
	{
		HashTableEntry<T> *entry = findEntry(key);
		if (entry->isOccupied)
		{
			result = makeSuccess(&entry->value);
		}
	}

	return result;
}

template<typename T>
Maybe<T> HashTable<T>::findValue(String key)
{
	Maybe<T> result = makeFailure<T>();

	if (entries != null)
	{
		HashTableEntry<T> *entry = findEntry(key);
		if (entry->isOccupied)
		{
			result = makeSuccess(entry->value);
		}
	}

	return result;
}

template<typename T>
T *HashTable<T>::findOrAdd(String key)
{
	HashTableEntry<T> *entry = findOrAddEntry(key);
	if (!entry->isOccupied)
	{
		count++;
		entry->isOccupied = true;
		entry->isGravestone = false;

		String theKey = pushString(&keyDataArena, key);
		hashString(&theKey);
		entry->key = theKey;
	}

	return &entry->value;
}

template<typename T>
T *HashTable<T>::put(String key, T value)
{
	HashTableEntry<T> *entry = findOrAddEntry(key);

	if (!entry->isOccupied)
	{
		count++;
		entry->isOccupied = true;
		entry->isGravestone = false;

		String theKey = pushString(&keyDataArena, key);
		hashString(&theKey);
		entry->key = theKey;
	}

	entry->value = value;

	return &entry->value;
}

template<typename T>
void HashTable<T>::putAll(HashTable<T> *source)
{
	for (auto it = source->iterate(); it.hasNext(); it.next())
	{
		auto entry = it.getEntry();
		put(entry->key, entry->value);
	}
}

template<typename T>
void HashTable<T>::removeKey(String key)
{
	if (entries == null) return;

	HashTableEntry<T> *entry = findEntry(key);
	if (entry->isOccupied)
	{
		entry->isGravestone = true;
		entry->isOccupied = false;
		count--;
	}
}

template<typename T>
void HashTable<T>::clear()
{
	ASSERT(!hasFixedMemory);

	if (count > 0)
	{
		count = 0;
		if (entries != null)
		{
			for (s32 i=0; i < capacity; i++)
			{
				entries[i] = {};
			}
		}
	}
}

template<typename T>
HashTableIterator<T> HashTable<T>::iterate()
{
	HashTableIterator<T> iterator = {};

	iterator.hashTable = this;
	iterator.currentIndex = 0;

	// If the table is empty, we can skip some work.
	iterator.isDone = (count == 0);

	// If the first entry is unoccupied, we need to skip ahead
	if (!iterator.isDone && !iterator.getEntry()->isOccupied)
	{
		iterator.next();
	}

	return iterator;
}

template<typename T>
void HashTable<T>::expand(s32 newCapacity)
{
	ASSERT(!hasFixedMemory);
	ASSERT(newCapacity > 0); //, "Attempted to resize a hash table to {0}", {formatInt(newCapacity)});
	ASSERT(newCapacity > capacity); //, "Attempted to shrink a hash table from {0} to {1}", {formatInt(capacity), formatInt(newCapacity)});

	s32 oldCount = count;
	s32 oldCapacity = capacity;
	HashTableEntry<T> *oldItems = entries;

	capacity = newCapacity;
	entries = (HashTableEntry<T>*) allocateRaw(newCapacity * sizeof(HashTableEntry<T>));
	count = 0;

	if (oldItems != null)
	{
		// Migrate old entries over
		for (s32 i=0; i < oldCapacity; i++)
		{
			HashTableEntry<T> *oldEntry = oldItems + i;

			if (oldEntry->isOccupied)
			{
				put(oldEntry->key, oldEntry->value);
			}
		}

		deallocateRaw(oldItems);
	}

	ASSERT(oldCount == count);//, "Hash table item count changed while expanding it! Old: {0}, new: {1}", {formatInt(oldCount), formatInt(table->count)});
}

template<typename T>
void HashTableIterator<T>::next()
{
	while (!isDone)
	{
		currentIndex++;

		if (currentIndex >= hashTable->capacity)
		{
			isDone = true;
		}
		else
		{
			// Only stop iterating if we find an occupied entry
			if (getEntry()->isOccupied)
			{
				break;
			}
		}
	}
}

template<typename T>
inline bool HashTableIterator<T>::hasNext()
{
	return !isDone;
}

template<typename T>
HashTableEntry<T> *HashTableIterator<T>::getEntry()
{
	return hashTable->entries + currentIndex;
}

template<typename T>
T *HashTableIterator<T>::get()
{
	return &getEntry()->value;
}
