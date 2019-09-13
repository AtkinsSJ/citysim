#pragma once

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, MemoryArena *arena, s32 itemsPerChunk)
{
	array->memoryArena = arena;
	array->chunkPool = null;
	array->itemsPerChunk = itemsPerChunk;
	array->chunkCount = 0;
	array->count = 0;
	array->firstChunk = null;
	array->lastChunk = null;

	appendChunk(array);
}

template<typename T>
void initChunkedArray(ChunkedArray<T> *array, ArrayChunkPool<T> *pool)
{
	array->memoryArena = null;
	array->chunkPool = pool;
	array->itemsPerChunk = pool->itemsPerChunk;
	array->chunkCount = 0;
	array->count = 0;
	array->firstChunk = null;
	array->lastChunk = null;
}

template<typename T>
void clear(ChunkedArray<T> *array)
{
	array->count = 0;
	for (ArrayChunk<T> *chunk = array->firstChunk; chunk; chunk = chunk->nextChunk)
	{
		chunk->count = 0;
	}

	if (array->chunkPool != null)
	{
		while (array->chunkCount > 0)
		{
			returnLastChunkToPool(array);
		}
	}
}

template<typename T>
ArrayChunk<T> *allocateChunk(MemoryArena *arena, s32 itemsPerChunk)
{
	// Rolled into a single allocation
	Blob blob = allocateBlob(arena, sizeof(ArrayChunk<T>) + (sizeof(T) * itemsPerChunk));
	ArrayChunk<T> *newChunk = (ArrayChunk<T> *)blob.memory;
	*newChunk = {};
	newChunk->count = 0;
	newChunk->items = (T *)(blob.memory + sizeof(ArrayChunk<T>));

	return newChunk;
}

template<typename T>
void appendChunk(ChunkedArray<T> *array)
{
	ArrayChunk<T> *newChunk = null;
	
	// Attempt to get a chunk from the pool if we can
	if (array->chunkPool != null)
	{
		newChunk = getItemFromPool(array->chunkPool);
	}
	else
	{
		newChunk = allocateChunk<T>(array->memoryArena, array->itemsPerChunk);
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
T *appendUninitialised(ChunkedArray<T> *array)
{
	bool useLastChunk = (array->count >= array->itemsPerChunk * (array->chunkCount-1));
	if (array->count >= (array->itemsPerChunk * array->chunkCount))
	{
		appendChunk(array);
		useLastChunk = true;
	}

	// Shortcut to the last chunk, because that's what we want 99% of the time!
	ArrayChunk<T> *chunk = null;
	if (useLastChunk)
	{
		chunk = array->lastChunk;
	}
	else
	{
		chunk = array->firstChunk;
		s32 indexWithinChunk = array->count;
		while (indexWithinChunk >= array->itemsPerChunk)
		{
			chunk = chunk->nextChunk;
			indexWithinChunk -= array->itemsPerChunk;
		}
	}

	array->count++;

	T *result = chunk->items + chunk->count++;

	return result;
}

template<typename T>
inline T *append(ChunkedArray<T> *array, T item)
{
	T *result = appendUninitialised(array);
	*result = item;
	return result;
}

template<typename T>
inline T *appendBlank(ChunkedArray<T> *array)
{
	T *result = appendUninitialised(array);
	*result = {};
	return result;
}

template<typename T>
T *get(ChunkedArray<T> *array, s32 index)
{
	ASSERT(index < array->count); //Index out of array bounds!

	T *result = null;

	s32 chunkIndex = index / array->itemsPerChunk;
	s32 itemIndex  = index % array->itemsPerChunk;

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
		ArrayChunk<T> *chunk = array->firstChunk;
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
ArrayChunk<T> *getChunkByIndex(ChunkedArray<T> *array, s32 chunkIndex)
{
	ASSERT(chunkIndex >= 0 && chunkIndex < array->chunkCount); //chunkIndex is out of range!

	ArrayChunk<T> *chunk = null;

	// Shortcuts for known values
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
ArrayChunk<T> *getLastNonEmptyChunk(ChunkedArray<T> *array)
{
	ArrayChunk<T> *lastNonEmptyChunk = array->lastChunk;

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
void moveItemKeepingOrder(ChunkedArray<T> *array, s32 fromIndex, s32 toIndex)
{
	DEBUG_FUNCTION();

	// Skip if there's nothing to do
	if (fromIndex == toIndex)  return;

	if (fromIndex < toIndex)
	{
		// Moving >, so move each item in the range left 1
		s32 chunkIndex = fromIndex / array->itemsPerChunk;
		s32 itemIndex  = fromIndex % array->itemsPerChunk;
		ArrayChunk<T> *chunk = getChunkByIndex(array, chunkIndex);

		T movingItem = chunk->items[itemIndex];

		for (s32 currentPosition = fromIndex; currentPosition < toIndex; currentPosition++)
		{
			T *dest = &chunk->items[itemIndex];

			itemIndex++;
			if (itemIndex >= array->itemsPerChunk)
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
		s32 chunkIndex = fromIndex / array->itemsPerChunk;
		s32 itemIndex  = fromIndex % array->itemsPerChunk;
		ArrayChunk<T> *chunk = getChunkByIndex(array, chunkIndex);
		
		T movingItem = chunk->items[itemIndex];

		for (s32 currentPosition = fromIndex; currentPosition > toIndex; currentPosition--)
		{
			T *dest = &chunk->items[itemIndex];
			itemIndex--;
			if (itemIndex < 0)
			{
				chunk = chunk->prevChunk;
				itemIndex = array->itemsPerChunk - 1;
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

	for (ArrayChunk<T> *chunk = array->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		for (s32 i=0; i<chunk->count; i++)
		{
			if (equals(chunk->items[i], toRemove))
			{
				// FOUND IT!
				found = true;

				ArrayChunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);

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
T removeIndex(ChunkedArray<T> *array, s32 indexToRemove, bool keepItemOrder)
{
	DEBUG_FUNCTION();

	ASSERT(indexToRemove >= 0 && indexToRemove < array->count); //indexToRemove is out of range!

	T result;
	
	ArrayChunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);

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
		s32 chunkIndex = indexToRemove / array->itemsPerChunk;
		s32 itemIndex  = indexToRemove % array->itemsPerChunk;

		ArrayChunk<T> *chunk = getChunkByIndex(array, chunkIndex);

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
void reserve(ChunkedArray<T> *array, s32 desiredSize)
{
	DEBUG_FUNCTION();
	
	while (((array->itemsPerChunk * array->chunkCount) - array->count) < desiredSize)
	{
		appendChunk(array);
	}
}

//////////////////////////////////////////////////
// POOL STUFF                                   //
//////////////////////////////////////////////////

template<typename T>
void initChunkPool(ArrayChunkPool<T> *pool, MemoryArena *arena, s32 itemsPerChunk)
{
	pool->itemsPerChunk = itemsPerChunk;
	initPool<ArrayChunk<T>>(pool, arena, &allocateChunkFromPool, &pool->itemsPerChunk);
}

template<typename T>
ArrayChunk<T> *allocateChunkFromPool(MemoryArena *arena, void *userData)
{
	s32 itemsPerChunk = *((s32*)userData);
	return allocateChunk<T>(arena, itemsPerChunk);
}

template<typename T>
void returnLastChunkToPool(ChunkedArray<T> *array)
{
	ArrayChunk<T> *chunk = array->lastChunk;

	ASSERT(chunk->count == 0); //Attempting to return a non-empty chunk to the chunk pool!
	array->lastChunk = array->lastChunk->prevChunk;
	if (array->firstChunk == chunk) array->firstChunk = array->lastChunk;
	array->chunkCount--;

	addItemToPool<ArrayChunk<T>>(chunk, array->chunkPool);
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////
template<typename T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, s32 initialIndex, bool wrapAround, bool goBackwards)
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
		iterator.chunkIndex   = initialIndex / array->itemsPerChunk;
		iterator.currentChunk = getChunkByIndex(array, iterator.chunkIndex);
		iterator.indexInChunk = initialIndex % array->itemsPerChunk;
	}

	return iterator;
}

template<typename T>
bool hasNext(ChunkedArrayIterator<T> *iterator)
{
	return !iterator->isDone;
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
			else
			{
				iterator->indexInChunk = iterator->currentChunk->count - 1;
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

	ASSERT(iterator->isDone || iterator->indexInChunk >= 0 && iterator->indexInChunk < iterator->currentChunk->count); //Bounds check
}

template<typename T>
inline T *get(ChunkedArrayIterator<T> iterator)
{
	return &iterator.currentChunk->items[iterator.indexInChunk];
}

template<typename T>
s32 getIndex(ChunkedArrayIterator<T> iterator)
{
	return (iterator.chunkIndex * iterator.array->itemsPerChunk) + iterator.indexInChunk;
}

template<typename T>
inline T getValue(ChunkedArrayIterator<T> iterator)
{
	return iterator.currentChunk->items[iterator.indexInChunk];
}
