#pragma once

template<typename T>
struct Set
{
	// TODO: @Speed: This is a really dumb, linearly-compare-with-all-the-elements implementation!
	// Replace it with a binary tree or some kind of hash map or something, if we end up using this a lot.
	// For now, simple is better!
	// - Sam, 03/09/2019
	ChunkedArray<T> items;
	bool (*areItemsEqual)(T *a, T *b);
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
};

template<typename T>
SetIterator<T> iterate(Set<T> *set)
{
	SetIterator<T> result = {};
	result.itemsIterator = iterate(&set->items);

	return result;
}

template<typename T>
void next(SetIterator<T> *iterator)
{
	next(&iterator->itemsIterator);
}

template<typename T>
bool isDone(SetIterator<T> *iterator)
{
	return iterator->itemsIterator.isDone;
}

template<typename T>
T *get(SetIterator<T> *iterator)
{
	return get(iterator->itemsIterator);
}

template<typename T>
T getValue(SetIterator<T> *iterator)
{
	return getValue(iterator->itemsIterator);
}
