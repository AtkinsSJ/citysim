#pragma once

template<class T>
Chunk<T> *getChunkByIndex(ChunkedArray<T> *array, umm chunkIndex)
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

	if (array->itemCount > 0)
	{
		while (lastNonEmptyChunk->count == 0)
		{
			lastNonEmptyChunk = lastNonEmptyChunk->prevChunk;
		}
	}

	return lastNonEmptyChunk;
}

template<class T>
bool findAndRemove(ChunkedArray<T> *array, T toRemove)
{
	bool found = false;

	for (Chunk<T> *chunk = &array->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		for (umm i=0; i<chunk->count; i++)
		{
			if (equals(chunk->items[i], toRemove))
			{
				// FOUND IT!
				found = true;

				Chunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);

				// Now, to copy the last element in the array to this position
				chunk->items[i] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
				lastNonEmptyChunk->count--;
				array->itemCount--;

				break;
			}
		}

		if (found) break;
	}

	return found;
}

template<class T>
T removeIndex(ChunkedArray<T> *array, umm indexToRemove)
{
	ASSERT(indexToRemove < array->itemCount, "indexToRemove is out of range!");

	T result;

	umm chunkIndex = indexToRemove / array->chunkSize;
	umm itemIndex  = indexToRemove % array->chunkSize;

	Chunk<T> *chunk = getChunkByIndex(array, chunkIndex);

	result = chunk->items[itemIndex];

	// Copy last item to overwrite this one
	Chunk<T> *lastNonEmptyChunk = getLastNonEmptyChunk(array);
	chunk->items[i] = lastNonEmptyChunk->items[lastNonEmptyChunk->count-1];
	lastNonEmptyChunk->count--;
	array->itemCount--;

	return result;
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

/**
 * This will iterate through every item in the array, starting at whichever initialIndex
 * you specify (default to the beginning). It wraps when it reaches the end.
 * Example usage:

	for (auto it = iterate(&array, randomInRange(random, array.itemCount));
		!it.isDone;
		next(&it))
	{
		auto thing = get(it);
		// do stuff with the thing
	}
 */
template<class T>
ChunkedArrayIterator<T> iterate(ChunkedArray<T> *array, umm initialIndex = 0)
{
	ChunkedArrayIterator<T> iterator = {};

	iterator.array = array;
	iterator.itemsIterated = 0;
	iterator.isDone = false;

	iterator.currentChunk = getChunkByIndex(array, initialIndex / array->chunkSize);
	iterator.indexInChunk = initialIndex % array->chunkSize;

	return iterator;
}

template<class T>
void next(ChunkedArrayIterator<T> *iterator)
{
	if (iterator->isDone) return;

	iterator->indexInChunk++;
	iterator->itemsIterated++;

	if (iterator->itemsIterated >= iterator->array->itemCount)
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
			// Wrap to the beginning!
			iterator->currentChunk = &iterator->array->firstChunk;
		}
	}
}

template<class T>
T get(ChunkedArrayIterator<T> iterator)
{
	return iterator.currentChunk->items[iterator.indexInChunk];
}