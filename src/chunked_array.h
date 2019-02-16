#pragma once

/**
 * Variation on Array where we allocate the data in chunks, which are linked-listed together.
 * This means we can allocate them within a memory arena, and have more control there.
 * BUT, it also means the array won't be contiguious after allocating a second chunk.
 */

template<class T>
struct Chunk
{
	smm count;
	smm maxCount;
	T *items;

	Chunk<T> *prevChunk;
	Chunk<T> *nextChunk;
};

template<class T>
struct ChunkedArray
{
	MemoryArena *memoryArena;

	smm chunkSize;
	smm chunkCount;
	smm count;

	Chunk<T> firstChunk;
	Chunk<T> *lastChunk;
};

template<class T>
struct ChunkedArrayIterator
{
	ChunkedArray<T> *array;

	Chunk<T> *currentChunk;
	smm indexInChunk;

	// This is a counter for use when we start not at the beginning of the array but want to iterate it ALL.
	// For simplicity, we increment it each time we next(), and when it equals the count, we're done.
	smm itemsIterated;
	bool isDone;
};

// markFirstChunkAsFull is a little hacky. It sets the count to be chunkSize, so that
// we can immediately get() those elements by index instead of having to append() to add them.
template<class T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, smm chunkSize, bool markFirstChunkAsFull = false)
{
	array->memoryArena = arena;
	array->chunkSize = chunkSize;
	array->chunkCount = 1;
	array->count = 0;

	array->firstChunk.count = 0;
	array->firstChunk.maxCount = chunkSize;
	array->firstChunk.items = PushArray(arena, T, chunkSize);
	array->firstChunk.nextChunk = null;
	array->firstChunk.prevChunk = null;

	array->lastChunk = &array->firstChunk;

	if (markFirstChunkAsFull)
	{
		array->count = chunkSize;
		array->firstChunk.count = chunkSize;
	}
}

// Doesn't free any memory, just marks all the chunks as empty.
template<class T>
void clear(ChunkedArray<T> *array)
{
	array->count = 0;
	for (Chunk<T> *chunk = &array->firstChunk; chunk; chunk = chunk->nextChunk)
	{
		chunk->count = 0;
	}
}

template<class T>
void appendChunk(ChunkedArray<T> *array)
{
	Chunk<T> *newChunk = PushStruct(array->memoryArena, Chunk<T>);
	newChunk->count = 0;
	newChunk->maxCount = array->chunkSize;
	newChunk->items = PushArray(array->memoryArena, T, array->chunkSize);
	newChunk->prevChunk = array->lastChunk;
	newChunk->nextChunk = null;

	array->chunkCount++;
	array->lastChunk->nextChunk = newChunk;
	array->lastChunk = newChunk;
}

template<class T>
T *append(ChunkedArray<T> *array, T item)
{
	bool useLastChunk = (array->count >= array->chunkSize * (array->chunkCount-1));
	if (array->count >= (array->chunkSize * array->chunkCount))
	{
		appendChunk(array);
		useLastChunk = true;
	}

	// Shortcut to the last chunk, because that's what we want 99% of the time!
	Chunk<T> *chunk = null;
	if (useLastChunk)
	{
		chunk = array->lastChunk;
	}
	else
	{
		chunk = &array->firstChunk;
		smm indexWithinChunk = array->count;
		while (indexWithinChunk >= array->chunkSize)
		{
			chunk = chunk->nextChunk;
			indexWithinChunk -= array->chunkSize;
		}
	}

	array->count++;

	T *result = chunk->items + chunk->count++;
	*result = item;

	return result;
}

template<class T>
T *appendBlank(ChunkedArray<T> *array)
{
	T *result = append(array, {});
	return result;
}

template<class T>
T *get(ChunkedArray<T> *array, smm index)
{
	ASSERT(index < array->count, "Index out of array bounds!");

	T *result = null;

	smm chunkIndex = index / array->chunkSize;
	smm itemIndex  = index % array->chunkSize;

	if (chunkIndex == 0)
	{
		// Early out!
		result = array->firstChunk.items + itemIndex;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		// Early out!
		result = array->lastChunk->items + itemIndex;
	}
	else
	{
		// Walk the chunk chain
		Chunk<T> *chunk = &array->firstChunk;
		while (chunkIndex > 0)
		{
			chunkIndex--;
			chunk = chunk->nextChunk;
		}

		result = chunk->items + itemIndex;
	}

	return result;
}

template<typename T>
void reserve(ChunkedArray<T> *array, smm desiredSize)
{
	while ((array->chunkSize * array->chunkCount) < desiredSize)
	{
		appendChunk(array);
	}
}