#pragma once

//
// It's a Hash Table! Keys go in, items go out
//
// We have a couple of different ways of creating one:
// 1) initHashTable() - the HashTable will manage its own memory, and will grow to make room
// 2) allocateFixedSizeHashTable() - The HashTable's memory is allocated from an Arena, and it won't grow
// 3) initFixedSizeHashTable() - Like 2, except you manually pass the memory in, so you can get it from
//    anywhere. Use calculateHashTableDataSize() to do the working out for you.
//

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
	bool hasFixedMemory; // Fixed-memory HashTables don't expand in size
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
	HashTableEntry<T> *findOrAddEntry(String key);
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

template <typename T>
HashTable<T> allocateFixedSizeHashTable(MemoryArena *arena, s32 capacity, f32 maxLoadFactor = 0.75f);

template <typename T>
smm calculateHashTableDataSize(s32 capacity, f32 maxLoadFactor);

template <typename T>
void initFixedSizeHashTable(HashTable<T> *table, s32 capacity, f32 maxLoadFactor, Blob entryData);

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
