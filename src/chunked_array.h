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

// Methods
	bool isEmpty();
	T *get(s32 index);

	T *append(T item);
	T *appendBlank();
	void reserve(s32 desiredSize);

	// Marks all the chunks as empty. Returns them to a chunkpool if there is one.
	void clear();

	// NB: Returns an Indexed<> of null/-1 if nothing is found.
	// A Maybe<> would be more specific, but Maybe<Indexed<T*>> is heading into ridiculousness territory.
	// Filter is a function with signature: bool filter(T *value)
	template <typename Filter>
	Indexed<T *> findFirst(Filter filter);

	bool findAndRemove(T toRemove);
	void removeIndex(s32 indexToRemove, bool keepItemOrder=false);
	template<typename Filter>
	s32 removeAll(Filter filter, s32 limit = -1);

	// Inserts the item at fromIndex into toIndex, moving other items around as necessary
	void moveItemKeepingOrder(s32 fromIndex, s32 toIndex);

	template <typename Comparison>
	void sort(Comparison compareElements);

// "private"
	T *appendUninitialised();

	void appendChunk();
	ArrayChunk<T> *getChunkByIndex(s32 chunkIndex);
	ArrayChunk<T> *getLastNonEmptyChunk();

	void returnLastChunkToPool();

	template <typename Comparison>
	void sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex);
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
		hasNext(&it);
		next(&it))
	{
		auto thing = get(&it);
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
T *get(ChunkedArrayIterator<T> *iterator);

template<typename T>
s32 getIndex(ChunkedArrayIterator<T> *iterator);

template<typename T>
T getValue(ChunkedArrayIterator<T> *iterator);