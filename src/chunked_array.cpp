#pragma once

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, smm chunkSize)
{
	array->memoryArena = arena;
	array->chunkPool = null;
	array->chunkSize = chunkSize;
	array->chunkCount = 0;
	array->count = 0;
	array->firstChunk = null;
	array->lastChunk = null;

	appendChunk(array);
}

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, ChunkPool<T> *pool)
{
	array->memoryArena = null;
	array->chunkPool = pool;
	array->chunkSize = pool->chunkSize;
	array->chunkCount = 0;
	array->count = 0;
	array->firstChunk = null;
	array->lastChunk = null;
}

template<typename T>
void clear(ChunkedArray<T> *array)
{
	array->count = 0;
	for (Chunk<T> *chunk = array->firstChunk; chunk; chunk = chunk->nextChunk)
	{
		chunk->count = 0;
	}
}

template<typename T>
Chunk<T> *allocateChunk(MemoryArena *arena, smm chunkSize)
{
	// Rolled into a single allocation
	Blob blob = allocateBlob(arena, sizeof(Chunk<T>) + (sizeof(T) * chunkSize));
	Chunk<T> *newChunk = (Chunk<T> *)blob.memory;
	*newChunk = {};
	newChunk->count = 0;
	newChunk->maxCount = chunkSize;
	newChunk->items = (T *)(blob.memory + sizeof(Chunk<T>));

	return newChunk;
}

template<typename T>
void appendChunk(ChunkedArray<T> *array)
{
	Chunk<T> *newChunk = null;
	
	// Attempt to get a chunk from the pool if we can
	if (array->chunkPool != null)
	{
		newChunk = getChunkFromPool(array->chunkPool);
	}
	else
	{
		newChunk = allocateChunk<T>(array->memoryArena, array->chunkSize);
	}
	newChunk->prevChunk = array->lastChunk;
	newChunk->nextChunk = null;

	array->chunkCount++;
	if (array->lastChunk != null)
	{
		array->lastChunk->nextChunk = newChunk;
	}
	array->lastChunk = newChunk;
	if (array->firstChunk == null)
	{
		array->firstChunk = newChunk;
	}
}

template<typename T>
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
		chunk = array->firstChunk;
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

template<typename T>
T *get(ChunkedArray<T> *array, smm index)
{
	ASSERT(index < array->count, "Index out of array bounds!");

	T *result = null;

	smm chunkIndex = index / array->chunkSize;
	smm itemIndex  = index % array->chunkSize;

	if (chunkIndex == 0)
	{
		// Early out!
		result = array->firstChunk->items + itemIndex;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		// Early out!
		result = array->lastChunk->items + itemIndex;
	}
	else
	{
		// Walk the chunk chain
		Chunk<T> *chunk = array->firstChunk;
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
Chunk<T> *getChunkByIndex(ChunkedArray<T> *array, smm chunkIndex)
{
	ASSERT(chunkIndex < array->chunkCount, "chunkIndex is out of range!");

	Chunk<T> *chunk = null;

	if (chunkIndex == 0)
	{
		chunk = array->firstChunk;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		chunk = array->lastChunk;
	}
	else
	{
		// Walk the chunk chain
		chunk = array->firstChunk;
		while (chunkIndex > 0)
		{
			chunkIndex--;
			chunk = chunk->nextChunk;
		}
	}

	return chunk;
}

template<typename T>
Chunk<T> *getLastNonEmptyChunk(ChunkedArray<T> *array)
{
	Chunk<T> *lastNonEmptyChunk = array->lastChunk;

	if (array->count > 0)
	{
		while (lastNonEmptyChunk->count == 0)
		{
			lastNonEmptyChunk = lastNonEmptyChunk->prevChunk;
		}
	}

	return lastNonEmptyChunk;
}

template<typename T>
void moveItemKeepingOrder(ChunkedArray<T> *array, smm fromIndex, smm toIndex)
{
	DEBUG_FUNCTION();

	// Skip if there's nothing to do
	if (fromIndex == toIndex)  return;

	if (fromIndex < toIndex)
	{
		// Moving >, so move each item in the range left 1
		smm chunkIndex = fromIndex / array->chunkSize;
		smm itemIndex  = fromIndex % array->chunkSize;
		Chunk<T> *chunk = getChunkByIndex(array, chunkIndex);

		T movingItem = chunk->items[itemIndex];

		for (smm currentPosition = fromIndex; currentPosition < toIndex; currentPosition++)
		{
			T *dest = &chunk->items[itemIndex];

			itemIndex++;
			if (itemIndex >= array->chunkSize)
			{
				chunk = chunk->nextChunk;
				itemIndex = 0;
			}

			T *src  = &chunk->items[itemIndex];

			*dest = *src;
		}

		chunk->items[itemIndex] = movingItem;
	}
	else
	{
		// Moving <, so move each item in the range right 1
		smm chunkIndex = fromIndex / array->chunkSize;
		smm itemIndex  = fromIndex % array->chunkSize;
		Chunk<T> *chunk = getChunkByIndex(array, chunkIndex);
		
		T movingItem = chunk->items[itemIndex];

		for (smm currentPosition = fromIndex; currentPosition > toIndex; currentPosition--)
		{
			T *dest = &chunk->items[itemIndex];
			itemIndex--;
			if (itemIndex < 0)
			{
				chunk = chunk->prevChunk;
				itemIndex = array->chunkSize - 1;
			}
			T *src = &chunk->items[itemIndex];

			*dest = *src;
		}

		chunk->items[itemIndex] = movingItem;
	}
}

template<typename T>
bool findAndRemove(ChunkedArray<T> *array, T toRemove)
{
	DEBUG_FUNCTION();

	bool found = false;

	for (Chunk<T> *chunk = array->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		for (smm i=0; i<chunk->count; i++)
		{
			if (equals(chunk->items[i], toRemove))
			{
				// FOUND IT!
				found = true;

				Chunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);

				// Now, to copy the last element in the array to this position
				chunk->items[i] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
				lastNonEmptyChunk->count--;
				array->count--;

				break;
			}
		}

		if (found) break;
	}

	// Return empty chunks to the chunkpool
	if (found && (array->chunkPool != null) && (array->lastChunk->count == 0))
	{
		returnLastChunkToPool(array);
	}

	return found;
}

template<typename T>
T removeIndex(ChunkedArray<T> *array, smm indexToRemove, bool keepItemOrder)
{
	DEBUG_FUNCTION();

	ASSERT(indexToRemove >= 0 && indexToRemove < array->count, "indexToRemove is out of range!");

	T result;
	
	Chunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);

	if (keepItemOrder)
	{
		if (indexToRemove != (array->count-1))
		{
			// NB: This copies the item we're about to remove to the end of the array.
			// I guess if Item is large, this could be expensive unnecessarily?
			// Then again, we're also copying it to `result`, which we currently don't ever use, so meh.
			// - Sam, 8/2/2019
			moveItemKeepingOrder(array, indexToRemove, array->count-1);
		}
		result = lastNonEmptyChunk->items[lastNonEmptyChunk->count - 1];
	}
	else
	{
		smm chunkIndex = indexToRemove / array->chunkSize;
		smm itemIndex  = indexToRemove % array->chunkSize;

		Chunk<T> *chunk = getChunkByIndex(array, chunkIndex);

		result = chunk->items[itemIndex];

		// We don't need to rearrange things if we're removing the last item
		if (indexToRemove != array->count - 1)
		{
			// Copy last item to overwrite this one
			chunk->items[itemIndex] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
		}
	}

	lastNonEmptyChunk->count--;
	array->count--;

	// Return empty chunks to the chunkpool
	if ((array->chunkPool != null) && (array->lastChunk->count == 0))
	{
		returnLastChunkToPool(array);
	}

	return result;
}

template<typename T>
void reserve(ChunkedArray<T> *array, smm desiredSize)
{
	DEBUG_FUNCTION();
	
	while ((array->chunkSize * array->chunkCount) < desiredSize)
	{
		appendChunk(array);
	}
}

//////////////////////////////////////////////////
// POOL STUFF                                   //
//////////////////////////////////////////////////

template<typename T>
void initChunkPool(ChunkPool<T> *pool, MemoryArena *arena, smm chunkSize)
{
	pool->memoryArena = arena;
	pool->chunkSize = chunkSize;
	pool->count = 0;
	pool->firstChunk = null;
}

template<typename T>
Chunk<T> *getChunkFromPool(ChunkPool<T> *pool)
{
	Chunk<T> *newChunk = null;

	if (pool->count > 0)
	{
		newChunk = pool->firstChunk;
		pool->firstChunk = newChunk->nextChunk;
		pool->count--;
		if (pool->firstChunk != null)
		{
			pool->firstChunk->prevChunk = null;
		}
	}
	else
	{
		newChunk = allocateChunk<T>(pool->memoryArena, pool->chunkSize);
	}

	return newChunk;
}

template<typename T>
void returnLastChunkToPool(ChunkedArray<T> *array)
{
	Chunk<T> *chunk = array->lastChunk;
	ChunkPool<T> *pool = array->chunkPool;

	ASSERT(chunk->count == 0, "Attempting to return a non-empty chunk to the chunk pool!");
	array->lastChunk = array->lastChunk->prevChunk;
	if (array->firstChunk == chunk) array->firstChunk = array->lastChunk;
	array->chunkCount--;

	chunk->prevChunk = null;
	chunk->nextChunk = pool->firstChunk;
	if (pool->firstChunk != null)
	{
		pool->firstChunk->prevChunk = chunk;
	}
	pool->firstChunk = chunk;
	pool->count++;
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////
template<typename T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, smm initialIndex, bool wrapAround, bool goBackwards)
{
	ChunkedArrayIterator<T> iterator = {};

	iterator.array = array;
	iterator.itemsIterated = 0;
	iterator.wrapAround = wrapAround;
	iterator.goBackwards = goBackwards;

	// If the array is empty, we can skip some work.
	iterator.isDone = array->count == 0;

	if (!iterator.isDone)
	{
		iterator.chunkIndex   = initialIndex / array->chunkSize;
		iterator.currentChunk = getChunkByIndex(array, iterator.chunkIndex);
		iterator.indexInChunk = initialIndex % array->chunkSize;
	}

	return iterator;
}


template<typename T>
void next(ChunkedArrayIterator<T> *iterator)
{
	if (iterator->isDone) return;

	iterator->itemsIterated++;
	if (iterator->itemsIterated >= iterator->array->count)
	{
		// We've seen each index once
		iterator->isDone = true;
	}

	if (iterator->goBackwards)
	{
		iterator->indexInChunk--;

		if (iterator->indexInChunk < 0)
		{
			// Prev chunk
			iterator->currentChunk = iterator->currentChunk->prevChunk;
			iterator->chunkIndex--;

			if (iterator->currentChunk == null)
			{
				if (iterator->wrapAround)
				{
					// Wrap to the beginning!
					iterator->chunkIndex   = iterator->array->chunkCount - 1;
					iterator->currentChunk = iterator->array->lastChunk;
					iterator->indexInChunk = iterator->currentChunk->count - 1;
				}
				else
				{
					// We're not wrapping, so we're done
					iterator->isDone = true;
				}
			}
		}
	}
	else
	{
		iterator->indexInChunk++;

		if (iterator->indexInChunk >= iterator->currentChunk->count)
		{
			// Next chunk
			iterator->chunkIndex++;
			iterator->currentChunk = iterator->currentChunk->nextChunk;
			iterator->indexInChunk = 0;

			if (iterator->currentChunk == null)
			{
				if (iterator->wrapAround)
				{
					// Wrap to the beginning!
					iterator->chunkIndex = 0;
					iterator->currentChunk = iterator->array->firstChunk;
				}
				else
				{
					// We're not wrapping, so we're done
					iterator->isDone = true;
				}
			}
		}
	}
}

template<typename T>
inline T *get(ChunkedArrayIterator<T> iterator)
{
	return &iterator.currentChunk->items[iterator.indexInChunk];
}

template<typename T>
smm getIndex(ChunkedArrayIterator<T> iterator)
{
	return (iterator.chunkIndex * iterator.array->chunkSize) + iterator.indexInChunk;
}

template<typename T>
inline T getValue(ChunkedArrayIterator<T> iterator)
{
	return iterator.currentChunk->items[iterator.indexInChunk];
}
