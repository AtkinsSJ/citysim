#pragma once

template <typename T>
void initQueue(Queue<T> *queue, MemoryArena *arena, s32 chunkSize)
{
	*queue = {};

	queue->chunkSize = chunkSize;

	initPool<QueueChunk<T>>(&queue->chunkPool, arena, &allocateQueueChunk, &queue->chunkSize);

	// We're starting off with one chunk, even though it's empty. Perhaps we should wait
	// until we actually add something? I'm not sure.
	queue->startChunk = getItemFromPool(&queue->chunkPool);
	queue->endChunk = queue->startChunk;
}

template <typename T>
QueueChunk<T> *allocateQueueChunk(MemoryArena *arena, void *userData) {
	s32 chunkSize = *(s32*)userData;

	u8 *bytes = (u8*) allocate(arena, sizeof(QueueChunk<T>) + (sizeof(T) * chunkSize));
	QueueChunk<T> *newChunk = (QueueChunk<T> *)bytes;
	*newChunk = {};
	newChunk->items = (T *)(bytes + sizeof(QueueChunk<T>));

	return newChunk;
}

template <typename T>
bool Queue<T>::isEmpty()
{
	return (count == 0);
}

template <typename T>
T *Queue<T>::push(T item)
{
	if (endChunk == null)
	{
		// In case we don't yet have a chunk, or we became empty and removed it, add one.
		ASSERT(startChunk == null); // If we have a start but no end, something has gone very wrong!!!
		startChunk = getItemFromPool(&chunkPool);
		startChunk->count = 0;
		startChunk->startIndex = 0;
		endChunk = startChunk;
	}
	else if (endChunk->count == chunkSize)
	{
		// We're full, so get a new chunk
		QueueChunk<T> *newChunk = getItemFromPool(&chunkPool);
		newChunk->count = 0;
		newChunk->startIndex = 0;
		newChunk->prevChunk = endChunk;
		endChunk->nextChunk = newChunk;

		endChunk = newChunk;
	}

	T *result = endChunk->items + endChunk->startIndex + endChunk->count;
	endChunk->count++;
	count++;

	*result = item;

	return result;
}

template <typename T>
Maybe<T> Queue<T>::pop()
{
	Maybe<T> result = makeFailure<T>();

	if (count > 0)
	{
		result = makeSuccess(startChunk->items[startChunk->startIndex]);

		startChunk->count--;
		startChunk->startIndex++;
		count--;

		if (startChunk->count == 0)
		{
			// We're empty! So return it to the pool.
			// EXCEPT: If this is the only chunk, we might as well keep it and just clear it!
			if (startChunk == endChunk)
			{
				startChunk->startIndex = 0;
			}
			else
			{
				QueueChunk<T> *newStartChunk = startChunk->nextChunk;
				if (newStartChunk != null)
				{
					newStartChunk->prevChunk = null;
				}

				addItemToPool(startChunk, &chunkPool);
				startChunk = newStartChunk;

				// If we just removed the only chunk, make sure to clear the endChunk to match.
				if (startChunk == null)
				{
					endChunk = null;
				}
			}
		}
	}

	return result;
}
