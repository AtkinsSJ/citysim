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
	bool isInitialised();

	bool contains(String key);
	HashTableEntry<T> *findEntry(String key);
	T *find(String key);
	T findValue(String key);
	T *findOrAdd(String key);
	T *put(String key, T value={});
	void putAll(HashTable<T> *source);
	void removeKey(String key);
	void clear();

	HashTableIterator<T> iterate();

// "Private"
	void expand(s32 newCapacity);
};

template<typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor=0.75f, s32 initialCapacity=0);

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
