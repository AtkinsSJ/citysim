#pragma once

/**
 * Variation on Array where we allocate the data in chunks, which are linked-listed together.
 * This means we can allocate them within a memory arena, and have more control there.
 * BUT, it also means the array won't be contiguious after allocating a second chunk.
 */

template<class T>
struct Chunk
{
	umm count;
	umm maxCount;
	T *items;

	Chunk<T> *nextChunk;
};

template<class T>
struct ChunkedArray
{
	MemoryArena *memoryArena;

	umm chunkSize;
	umm chunkCount;
	umm itemCount;

	Chunk<T> firstChunk;
	Chunk<T> *lastChunk;
};

template<class T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, umm chunkSize)
{
	array->memoryArena = arena;
	array->chunkSize = chunkSize;
	array->chunkCount = 1;
	array->itemCount = 0;

	array->firstChunk.count = 0;
	array->firstChunk.maxCount = chunkSize;
	array->firstChunk.items = PushArray(arena, T, chunkSize);
	array->firstChunk.nextChunk = null;

	array->lastChunk = &array->firstChunk;
}

// Doesn't free any memory, just marks all the chunks as empty.
template<class T>
void clear(ChunkedArray<T> *array)
{
	array->itemCount = 0;
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
	newChunk->nextChunk = null;

	array->chunkCount++;
	array->lastChunk->nextChunk = newChunk;
	array->lastChunk = newChunk;
}

template<class T>
T *append(ChunkedArray<T> *array, T item)
{
	bool useLastChunk = (array->itemCount >= array->chunkSize * (array->chunkCount-1));
	if (array->itemCount >= (array->chunkSize * array->chunkCount))
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
		umm indexWithinChunk = array->itemCount;
		while (indexWithinChunk >= array->chunkSize)
		{
			chunk = chunk->nextChunk;
			indexWithinChunk -= array->chunkSize;
		}
	}
	
	array->itemCount++;

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
T *get(ChunkedArray<T> *array, umm index)
{
	ASSERT(index < array->itemCount, "Index out of array bounds!");

	T *result = null;

	umm chunkIndex = index / array->chunkSize;
	umm itemIndex  = index % array->chunkSize;

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