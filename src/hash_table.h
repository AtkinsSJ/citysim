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