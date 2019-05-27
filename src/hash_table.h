#pragma once

template<typename T>
struct HashTableEntry
{
	bool isOccupied;
	String key;
	
	T value;
};

template<typename T>
struct HashTable
{
	s32 count;
	s32 capacity;
	f32 maxLoadFactor;
	HashTableEntry<T> *entries;
};

template<typename T>
struct HashTableIterator
{
	HashTable<T> *hashTable;
	s32 currentIndex;

	bool isDone;
};

template<typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor=0.75f, s32 initialCapacity=0);

template<typename T>
T *find(HashTable<T> *table, String key);

// Adds a blank entry with the key if it can't find an existing one
template<typename T>
T *findOrAdd(HashTable<T> *table, String key);

template<typename T>
T findValue(HashTable<T> *table, String key);

template<typename T>
T *put(HashTable<T> *table, String key, T value={});

template<typename T>
void clear(HashTable<T> *table);

template<typename T>
void freeHashTable(HashTable<T> *table);

template<typename T>
HashTableIterator<T> iterate(HashTable<T> *table);

template<typename T>
void next(HashTableIterator<T> *iterator);

template<typename T>
HashTableEntry<T> *getEntry(HashTableIterator<T> iterator);

template<typename T>
T *get(HashTableIterator<T> iterator);