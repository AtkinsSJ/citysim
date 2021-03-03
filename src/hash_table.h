#pragma once

template<typename T>
struct HashTableEntry
{
	bool isOccupied;
	bool isGravestone; // Apparently the correct term is "tombstone", oops.

	String key;
	
	T value;
};

template<typename T>
struct HashTableIterator;

template<typename T>
struct HashTable
{
	s32 count;
	s32 capacity;
	f32 maxLoadFactor;
	HashTableEntry<T> *entries;

	// @Size: In a lot of cases, we already store the key in a separate StringTable, so having
	// it stored here too is redundant. But, keys are small so it's unlikely to cause any real
	// issues.
	// - Sam, 08/01/2020
	MemoryArena keyDataArena;

// Methods
	HashTableIterator<T> iterate();
};

template<typename T>
inline bool isHashTableInitialised(HashTable<T> *table)
{
	return (table->entries != null || table->maxLoadFactor > 0.0f);
}

template<typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor=0.75f, s32 initialCapacity=0);

template<typename T>
T *find(HashTable<T> *table, String key);

template<typename T>
bool contains(HashTable<T> *table, String key);

template<typename T>
HashTableEntry<T> *findEntry(HashTable<T> *table, String key);

// Adds a blank entry with the key if it can't find an existing one
template<typename T>
T *findOrAdd(HashTable<T> *table, String key);

template<typename T>
T findValue(HashTable<T> *table, String key);

template<typename T>
T *put(HashTable<T> *table, String key, T value={});

template<typename T>
void putAll(HashTable<T> *table, HashTable<T> *source);

template<typename T>
void removeKey(HashTable<T> *table, String key);

template<typename T>
void clear(HashTable<T> *table);

template<typename T>
void freeHashTable(HashTable<T> *table);

template<typename T>
struct HashTableIterator
{
	HashTable<T> *hashTable;
	s32 currentIndex;

	bool isDone;

// Methods
	bool hasNext();
	void next();
	T *get();
	HashTableEntry<T> *getEntry();
};
