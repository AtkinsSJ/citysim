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

	array->appendChunk();
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
inline bool ChunkedArray<T>::isEmpty()
{
	return count == 0;
}

template<typename T>
T *ChunkedArray<T>::get(s32 index)
{
	ASSERT(index >= 0 && index < count); //Index out of array bounds!

	T *result = null;

	s32 chunkIndex = index / itemsPerChunk;
	s32 itemIndex  = index % itemsPerChunk;

	if (chunkIndex == 0)
	{
		// Early out!
		result = firstChunk->items + itemIndex;
	}
	else if (chunkIndex == (chunkCount - 1))
	{
		// Early out!
		result = lastChunk->items + itemIndex;
	}
	else
	{
		// Walk the chunk chain
		ArrayChunk<T> *chunk = firstChunk;
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
void ChunkedArray<T>::clear()
{
	count = 0;
	for (ArrayChunk<T> *chunk = firstChunk; chunk; chunk = chunk->nextChunk)
	{
		chunk->count = 0;
	}

	if (chunkPool != null)
	{
		while (chunkCount > 0)
		{
			returnLastChunkToPool(this);
		}
	}
}

template <typename T>
T *ChunkedArray<T>::append(T item)
{
	T *result = appendUninitialised();
	*result = item;
	return result;
}

template <typename T>
T *ChunkedArray<T>::appendBlank()
{
	T *result = appendUninitialised();
	*result = {};
	return result;
}

template<typename T>
void ChunkedArray<T>::reserve(s32 desiredSize)
{
	DEBUG_FUNCTION();
	
	while (((itemsPerChunk * chunkCount) - count) < desiredSize)
	{
		appendChunk();
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
		ArrayChunk<T> *chunk = array->getChunkByIndex(chunkIndex);

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
		ArrayChunk<T> *chunk = array->getChunkByIndex(chunkIndex);
		
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

template<typename T, typename Filter>
s32 removeAll(ChunkedArray<T> *array, Filter filter, s32 limit)
{
	DEBUG_FUNCTION();

	s32 removedCount = 0;
	bool limited = (limit != -1);

	for (ArrayChunk<T> *chunk = array->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		for (s32 i=0; i<chunk->count; i++)
		{
			if (filter(chunk->items[i]))
			{
				// FOUND ONE!
				removedCount++;

				ArrayChunk<T> *lastNonEmptyChunk = array->getLastNonEmptyChunk();

				// Now, to copy the last element in the array to this position
				chunk->items[i] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
				lastNonEmptyChunk->count--;
				array->count--;

				if (limited && removedCount >= limit)
				{
					break;
				}
				else
				{
					// We just moved a different element into position i, so make sure we
					// check that one too!
					i--;
				}
			}
		}

		if (limited && removedCount >= limit)
		{
			break;
		}
	}

	// Return empty chunks to the chunkpool
	if (removedCount && (array->chunkPool != null))
	{
		while ((array->lastChunk != null) && (array->lastChunk->count == 0))
		{
			returnLastChunkToPool(array);
		}
	}

	return removedCount;
}

template<typename T, typename Filter>
Indexed<T *> findFirst(ChunkedArray<T> *array, Filter filter)
{
	Indexed<T*> result = makeNullIndexedValue<T*>();

	for (auto it = iterate(array); hasNext(&it); next(&it))
	{
		T *entry = get(&it);
		if (filter(entry))
		{
			result = makeIndexedValue(entry, getIndex(&it));
			break;
		}
	}

	return result;
}

template<typename T>
bool findAndRemove(ChunkedArray<T> *array, T toRemove)
{
	DEBUG_FUNCTION();

	s32 removed = removeAll(array, [&](T t) { return equals(t, toRemove); }, 1);

	return removed > 0;
}

template<typename T>
void removeIndex(ChunkedArray<T> *array, s32 indexToRemove, bool keepItemOrder)
{
	DEBUG_FUNCTION();

	if (indexToRemove < 0 || indexToRemove >= array->count)
	{
		logError("Attempted to remove non-existent index {0} from a ChunkedArray!"_s, {formatInt(indexToRemove)});
		return;
	}
	
	ArrayChunk<T> *lastNonEmptyChunk = array->getLastNonEmptyChunk();

	if (keepItemOrder)
	{
		if (indexToRemove != (array->count-1))
		{
			// NB: This copies the item we're about to remove to the end of the array.
			// I guess if Item is large, this could be expensive unnecessarily?
			// - Sam, 8/2/2019
			moveItemKeepingOrder(array, indexToRemove, array->count-1);
		}
	}
	else
	{
		s32 chunkIndex = indexToRemove / array->itemsPerChunk;
		s32 itemIndex  = indexToRemove % array->itemsPerChunk;

		ArrayChunk<T> *chunk = array->getChunkByIndex(chunkIndex);

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
	if ((array->chunkPool != null) && (array->lastChunk != null) && (array->lastChunk->count == 0))
	{
		returnLastChunkToPool(array);
	}
}

template<typename T, typename Comparison>
inline void sortChunkedArray(ChunkedArray<T> *array, Comparison compareElements)
{
	sortChunkedArrayInternal(array, compareElements, 0, array->count-1);
}

template<typename T, typename Comparison>
void sortChunkedArrayInternal(ChunkedArray<T> *array, Comparison compareElements, s32 lowIndex, s32 highIndex)
{
	// Quicksort implementation, copied from sortArrayInternal().
	// This is probably really terrible, because we're not specifically handling the chunked-ness,
	// so yeah. @Speed

	if (lowIndex < highIndex)
	{
		s32 partitionIndex = 0;
		{
			T *pivot = array->get(highIndex);
			s32 i = (lowIndex - 1);
			for (s32 j = lowIndex; j < highIndex; j++)
			{
				T *itemJ = array->get(j);
				if (compareElements(itemJ, pivot))
				{
					i++;
					T *itemI = array->get(i);
					T temp = {};

					temp = *itemI;
					*itemI = *itemJ;
					*itemJ = temp;
				}
			}
			
			T *itemIPlus1 = array->get(i+1);
			T *itemHighIndex = array->get(highIndex);
			T temp = {};

			temp = *itemIPlus1;
			*itemIPlus1 = *itemHighIndex;
			*itemHighIndex = temp;

			partitionIndex = i+1;
		}

		sortChunkedArrayInternal(array, compareElements, lowIndex, partitionIndex - 1);
		sortChunkedArrayInternal(array, compareElements, partitionIndex + 1, highIndex);
	}
}

template <typename T>
T *ChunkedArray<T>::appendUninitialised()
{
	bool useLastChunk = (count >= itemsPerChunk * (chunkCount-1));
	if (count >= (itemsPerChunk * chunkCount))
	{
		appendChunk();
		useLastChunk = true;
	}

	// Shortcut to the last chunk, because that's what we want 99% of the time!
	ArrayChunk<T> *chunk = null;
	if (useLastChunk)
	{
		chunk = lastChunk;
	}
	else
	{
		chunk = firstChunk;
		s32 indexWithinChunk = count;
		while (indexWithinChunk >= itemsPerChunk)
		{
			chunk = chunk->nextChunk;
			indexWithinChunk -= itemsPerChunk;
		}
	}

	count++;

	T *result = chunk->items + chunk->count++;

	return result;
}


template<typename T>
void ChunkedArray<T>::appendChunk()
{
	ArrayChunk<T> *newChunk = null;
	
	// Attempt to get a chunk from the pool if we can
	if (chunkPool != null)
	{
		newChunk = getItemFromPool(chunkPool);
	}
	else
	{
		newChunk = allocateChunk<T>(memoryArena, itemsPerChunk);
	}
	newChunk->prevChunk = lastChunk;
	newChunk->nextChunk = null;

	chunkCount++;
	if (lastChunk != null)
	{
		lastChunk->nextChunk = newChunk;
	}
	lastChunk = newChunk;
	if (firstChunk == null)
	{
		firstChunk = newChunk;
	}
}

template<typename T>
ArrayChunk<T> *ChunkedArray<T>::getChunkByIndex(s32 chunkIndex)
{
	ASSERT(chunkIndex >= 0 && chunkIndex < chunkCount); //chunkIndex is out of range!

	ArrayChunk<T> *chunk = null;

	// Shortcuts for known values
	if (chunkIndex == 0)
	{
		chunk = firstChunk;
	}
	else if (chunkIndex == (chunkCount - 1))
	{
		chunk = lastChunk;
	}
	else
	{
		// Walk the chunk chain
		chunk = firstChunk;
		while (chunkIndex > 0)
		{
			chunkIndex--;
			chunk = chunk->nextChunk;
		}
	}

	return chunk;
}

template<typename T>
ArrayChunk<T> *ChunkedArray<T>::getLastNonEmptyChunk()
{
	ArrayChunk<T> *lastNonEmptyChunk = lastChunk;

	if (count > 0)
	{
		while (lastNonEmptyChunk->count == 0)
		{
			lastNonEmptyChunk = lastNonEmptyChunk->prevChunk;
		}
	}

	return lastNonEmptyChunk;
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
		iterator.currentChunk = array->getChunkByIndex(iterator.chunkIndex);
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
inline T *get(ChunkedArrayIterator<T> *iterator)
{
	return &iterator->currentChunk->items[iterator->indexInChunk];
}

template<typename T>
s32 getIndex(ChunkedArrayIterator<T> *iterator)
{
	return (iterator->chunkIndex * iterator->array->itemsPerChunk) + iterator->indexInChunk;
}

template<typename T>
inline T getValue(ChunkedArrayIterator<T> *iterator)
{
	return iterator->currentChunk->items[iterator->indexInChunk];
}
