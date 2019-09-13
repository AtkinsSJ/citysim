#pragma once

/**
 * Variation on Array where we allocate the data in chunks, which are linked-listed together.
 * This means we can allocate them within a memory arena, and have more control there.
 * BUT, it also means the array won't be contiguious after allocating a second chunk.
 */

template<typename T>
struct ArrayChunk : PoolItem
{
	s32 count;
	T *items;

	ArrayChunk<T> *prevChunk;
	ArrayChunk<T> *nextChunk;
};

template<typename T>
struct ArrayChunkPool : Pool<ArrayChunk<T>>
{
	s32 itemsPerChunk;
};

template<typename T>
struct ChunkedArray
{
	ArrayChunkPool<T> *chunkPool;
	MemoryArena *memoryArena;

	s32 itemsPerChunk;
	s32 chunkCount;
	s32 count;

	ArrayChunk<T> *firstChunk;
	ArrayChunk<T> *lastChunk;
};

template<typename T>
struct ChunkedArrayIterator
{
	ChunkedArray<T> *array;
	bool wrapAround;
	bool goBackwards;

	ArrayChunk<T> *currentChunk;
	s32 chunkIndex;
	s32 indexInChunk;

	// This is a counter for use when we start not at the beginning of the array but want to iterate it ALL.
	// For simplicity, we increment it each time we next(), and when it equals the count, we're done.
	s32 itemsIterated;
	bool isDone;
};

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, s32 itemsPerChunk);

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, ArrayChunkPool<T> *pool);

template<typename T>
ArrayChunk<T> *allocateChunk(MemoryArena *arena, s32 itemsPerChunk);

// Marks all the chunks as empty. Returns them to a chunkpool if there is one.
template<typename T>
void clear(ChunkedArray<T> *array);

template<typename T>
T *append(ChunkedArray<T> *array, T item);

template<typename T>
T *appendBlank(ChunkedArray<T> *array);

template<typename T>
T *appendUninitialised(ChunkedArray<T> *array);

template<typename T>
T *get(ChunkedArray<T> *array, s32 index);

template<typename T>
void reserve(ChunkedArray<T> *array, s32 desiredSize);

template<typename T>
bool findAndRemove(ChunkedArray<T> *array, T toRemove);

template<typename T>
T removeIndex(ChunkedArray<T> *array, s32 indexToRemove, bool keepItemOrder);

template<typename T>
void moveItemKeepingOrder(ChunkedArray<T> *array, s32 fromIndex, s32 toIndex);

//////////////////////////////////////////////////
// POOL STUFF                                   //
//////////////////////////////////////////////////

template<typename T>
void initChunkPool(ArrayChunkPool<T> *pool, MemoryArena *arena, s32 itemsPerChunk);

// Used as Pool's allocateItem() function pointer
template<typename T>
ArrayChunk<T> *allocateChunkFromPool(MemoryArena *arena, void *userData);

template<typename T>
void returnLastChunkToPool(ChunkedArray<T> *array);

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////
/**
 * This will iterate through every item in the array, starting at whichever initialIndex
 * you specify (default to the beginning).
 * If wrapAround is true, the iterator will wrap from the end to the beginning so that
 * every item is iterated once. If false, we stop after the last item.
 * Example usage:

	for (auto it = iterate(&array, randomBelow(random, array.count), true);
		!it.isDone;
		next(&it))
	{
		auto thing = get(it);
		// do stuff with the thing
	}
 */

template<typename T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, s32 initialIndex = 0, bool wrapAround = true, bool goBackwards = false);

template<typename T>
inline ChunkedArrayIterator<T> iterateBackwards(ChunkedArray<T> *array)
{
	return iterate(array, array->count - 1, false, true);
}

template<typename T>
void next(ChunkedArrayIterator<T> *iterator);

template<typename T>
bool hasNext(ChunkedArrayIterator<T> *iterator);

template<typename T>
T *get(ChunkedArrayIterator<T> iterator);

template<typename T>
s32 getIndex(ChunkedArrayIterator<T> iterator);

template<typename T>
T getValue(ChunkedArrayIterator<T> iterator);