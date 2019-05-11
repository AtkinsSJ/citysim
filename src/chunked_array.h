#pragma once

/**
 * Variation on Array where we allocate the data in chunks, which are linked-listed together.
 * This means we can allocate them within a memory arena, and have more control there.
 * BUT, it also means the array won't be contiguious after allocating a second chunk.
 */

template<typename T>
struct Chunk
{
	smm count;
	smm maxCount;
	T *items;

	Chunk<T> *prevChunk;
	Chunk<T> *nextChunk;
};

template<typename T>
struct ChunkedArray
{
	MemoryArena *memoryArena;

	smm chunkSize;
	smm chunkCount;
	smm count;

	Chunk<T> firstChunk;
	Chunk<T> *lastChunk;
};

// markFirstChunkAsFull is a little hacky. It sets the count to be chunkSize, so that
// we can immediately get() those elements by index instead of having to append() to add them.
template<typename T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, smm chunkSize, bool markFirstChunkAsFull = false);

// Doesn't free any memory, just marks all the chunks as empty.
template<typename T>
void clear(ChunkedArray<T> *array);

template<typename T>
T *append(ChunkedArray<T> *array, T item);

template<typename T>
T *appendBlank(ChunkedArray<T> *array)
{
	T *result = append(array, {});
	return result;
}

template<typename T>
T *get(ChunkedArray<T> *array, smm index);

template<typename T>
void reserve(ChunkedArray<T> *array, smm desiredSize);

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////
/**
 * This will iterate through every item in the array, starting at whichever initialIndex
 * you specify (default to the beginning).
 * If wrapAround is true, the iterator will wrap from the end to the beginning so that
 * every item is iterated once. If false, we stop after the last item.
 * Example usage:

	for (auto it = iterate(&array, randomInRange(random, array.count), true);
		!it.isDone;
		next(&it))
	{
		auto thing = get(it);
		// do stuff with the thing
	}
 */

template<typename T>
struct ChunkedArrayIterator
{
	ChunkedArray<T> *array;

	Chunk<T> *currentChunk;
	smm indexInChunk;

	// This is a counter for use when we start not at the beginning of the array but want to iterate it ALL.
	// For simplicity, we increment it each time we next(), and when it equals the count, we're done.
	smm itemsIterated;
	bool wrapAround;
	bool isDone;
};

template<typename T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, smm initialIndex = 0, bool wrapAround = true);

template<typename T>
void next(ChunkedArrayIterator<T> *iterator);

template<typename T>
T *get(ChunkedArrayIterator<T> iterator);

template<typename T>
T getValue(ChunkedArrayIterator<T> iterator);