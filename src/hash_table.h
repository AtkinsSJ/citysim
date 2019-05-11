#pragma once

template<typename T>
struct HashTableEntry
{
	bool isOccupied;
	String key;
	u32 keyHash;
	
	T value;
};

template<typename T>
struct HashTable
{
	smm count;
	smm capacity;
	f32 maxLoadFactor;
	HashTableEntry<T> *entries;
};

template<typename T>
struct HashTableIterator
{
	HashTable<T> *hashTable;
	smm currentIndex;

	bool isDone;
};

template<typename T>
void initHashTable(HashTable<T> *table, f32 maxLoadFactor=0.75f, smm initialCapacity=0);

template<typename T>
T *find(HashTable<T> *table, String key);

template<typename T>
T findValue(HashTable<T> *table, String key);

template<typename T>
T *put(HashTable<T> *table, String key, T value={});

template<typename T>
void clear(HashTable<T> *table);

template<typename T>
HashTableIterator<T> iterate(HashTable<T> *table);

template<typename T>
void next(HashTableIterator<T> *iterator);

template<typename T>
HashTableEntry<T> *get(HashTableIterator<T> iterator);