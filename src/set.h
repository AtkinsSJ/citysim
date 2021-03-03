#pragma once

template<typename T>
struct SetIterator;

template<typename T>
struct Set
{
	// TODO: @Speed: This is a really dumb, linearly-compare-with-all-the-elements implementation!
	// Replace it with a binary tree or some kind of hash map or something, if we end up using this a lot.
	// For now, simple is better!
	// - Sam, 03/09/2019
	ChunkedArray<T> items;
	bool (*areItemsEqual)(T *a, T *b);

// Methods
	SetIterator<T> iterate();
};

template<typename T>
void initSet(Set<T> *set, MemoryArena *arena, bool (*areItemsEqual)(T *a, T *b) = [](T *a, T *b) { return *a == *b; });

// Returns true if the item got added (aka, it was not already in the set)
template<typename T>
bool add(Set<T> *set, T item);

template<typename T>
bool contains(Set<T> *set, T item);

template<typename T>
void clear(Set<T> *set);

template<typename T>
struct SetIterator
{
	// This is SUPER JANKY
	ChunkedArrayIterator<T> itemsIterator;

// Methods
	bool hasNext();
	void next();
	T *get();
	T getValue();
};

template<typename T>
SetIterator<T> iterate(Set<T> *set)
{
	return set->iterate();
}

template<typename T>
void next(SetIterator<T> *iterator) { iterator->next(); }

template<typename T>
bool hasNext(SetIterator<T> *iterator) { return iterator->hasNext(); }

template<typename T>
T *get(SetIterator<T> *iterator) { return iterator->get(); }

template<typename T>
T getValue(SetIterator<T> *iterator) { return iterator->getValue(); }
