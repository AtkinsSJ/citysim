#pragma once

template<class T>
Chunk<T> *getChunkByIndex(ChunkedArray<T> *array, smm chunkIndex)
{
	ASSERT(chunkIndex < array->chunkCount, "chunkIndex is out of range!");

	Chunk<T> *chunk = null;

	if (chunkIndex == 0)
	{
		chunk = &array->firstChunk;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		chunk = array->lastChunk;
	}
	else
	{
		// Walk the chunk chain
		chunk = &array->firstChunk;
		while (chunkIndex > 0)
		{
			chunkIndex--;
			chunk = chunk->nextChunk;
		}
	}

	return chunk;
}

template<class T>
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

template<class T>
void moveItemKeepingOrder(ChunkedArray<T> *array, smm fromIndex, smm toIndex)
{
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

template<class T>
bool findAndRemove(ChunkedArray<T> *array, T toRemove)
{
	bool found = false;

	for (Chunk<T> *chunk = &array->firstChunk;
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

	return found;
}

template<class T>
T removeIndex(ChunkedArray<T> *array, smm indexToRemove, bool keepItemOrder)
{
	ASSERT(indexToRemove < array->count, "indexToRemove is out of range!");

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
			chunk->items[indexToRemove] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
		}
	}

	lastNonEmptyChunk->count--;
	array->count--;

	return result;
}

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
template<class T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, smm initialIndex = 0, bool wrapAround = true)
{
	ChunkedArrayIterator<T> iterator = {};

	iterator.array = array;
	iterator.itemsIterated = 0;
	iterator.wrapAround = wrapAround;

	// If the array is empty, we can skip some work.
	iterator.isDone = array->count == 0;

	if (!iterator.isDone)
	{
		iterator.currentChunk = getChunkByIndex(array, initialIndex / array->chunkSize);
		iterator.indexInChunk = initialIndex % array->chunkSize;
	}

	return iterator;
}

template<class T>
void next(ChunkedArrayIterator<T> *iterator)
{
	if (iterator->isDone) return;

	iterator->indexInChunk++;
	iterator->itemsIterated++;

	if (iterator->itemsIterated >= iterator->array->count)
	{
		// We've seen each index once
		iterator->isDone = true;
	}

	if (iterator->indexInChunk >= iterator->currentChunk->count)
	{
		iterator->currentChunk = iterator->currentChunk->nextChunk;
		iterator->indexInChunk = 0;

		if (iterator->currentChunk == null)
		{
			if (iterator->wrapAround)
			{
				// Wrap to the beginning!
				iterator->currentChunk = &iterator->array->firstChunk;
			}
			else
			{
				// We're not wrapping, so we're done
				iterator->isDone = true;
			}
		}
	}
}

template<class T>
T *get(ChunkedArrayIterator<T> iterator)
{
	return &iterator.currentChunk->items[iterator.indexInChunk];
}

template<class T>
T getValue(ChunkedArrayIterator<T> iterator)
{
	return iterator.currentChunk->items[iterator.indexInChunk];
}
