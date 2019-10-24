 
template<typename T>
void initOccupancyArray(OccupancyArray<T> *array, MemoryArena *arena, s32 itemsPerChunk)
{
	array->memoryArena = arena;
	array->itemsPerChunk = itemsPerChunk;
	array->chunkCount = 0;
	array->count = 0;
	array->firstChunk = null;
	array->lastChunk = null;
	array->firstChunkWithSpace = null;
	array->firstChunkWithSpaceIndex = -1;
}

template<typename T>
Indexed<T*> append(OccupancyArray<T> *array)
{
	Indexed<T*> result = {};

	if (array->firstChunkWithSpace == null)
	{
		// Append a new chunk

		// @Copypasta ArrayChunk::allocateChunk(). We probably want to move this logic into
		// an "allocate a struct with its data array" function somehow? That's not easy in C++ though.
		// Actually, this one has two extra parts, so that's even more complicated...
		// - Sam, 21/08/2019
		smm structSize = sizeof(OccupancyArrayChunk<T>);
		smm arraySize = sizeof(T) * array->itemsPerChunk;
		s32 occupancyArrayCount = calculateBitArrayU64Count(array->itemsPerChunk);
		smm occupancyArraySize = occupancyArrayCount * sizeof(u64);

		Blob blob = allocateBlob(array->memoryArena, structSize + arraySize + occupancyArraySize);
		OccupancyArrayChunk<T> *newChunk = (OccupancyArrayChunk<T> *)blob.memory;
		*newChunk = {};
		newChunk->count = 0;
		newChunk->items = (T *)(blob.memory + structSize);
		initBitArray(&newChunk->occupancy, array->itemsPerChunk, makeArray(occupancyArrayCount, (u64 *)(blob.memory + structSize + arraySize)));

		array->chunkCount++;

		// Attach the chunk to the end, but also make it the "firstChunkWithSpace"
		if (array->firstChunk == null) array->firstChunk = newChunk;

		if (array->lastChunk != null)
		{
			array->lastChunk->nextChunk = newChunk;
			newChunk->prevChunk = array->lastChunk;
		}
		array->lastChunk = newChunk;

		array->firstChunkWithSpace = newChunk;
		array->firstChunkWithSpaceIndex = array->chunkCount - 1;
	}

	OccupancyArrayChunk<T> *chunk = array->firstChunkWithSpace;

	// getFirstUnsetBitIndex() to find the free slot
	s32 indexInChunk = getFirstUnsetBitIndex(&chunk->occupancy);
	ASSERT_PARANOID(indexInChunk >= 0 && indexInChunk < array->itemsPerChunk);

	// mark that slot as occupied
	setBit(&chunk->occupancy, indexInChunk);
	result.index = indexInChunk + (array->firstChunkWithSpaceIndex * array->itemsPerChunk);
	result.value = chunk->items + indexInChunk;

	// update counts
	chunk->count++;
	array->count++;

	if (chunk->count == array->itemsPerChunk)
	{
		// recalculate firstChunkWithSpace/Index
		if (array->count == array->itemsPerChunk * array->chunkCount)
		{
			// We're full! So just set the firstChunkWithSpace to null
			array->firstChunkWithSpace = null;
			array->firstChunkWithSpaceIndex = -1;
		}
		else
		{
			// There's a chunk with space, we just have to find it
			// Assuming that array->firstChunkWithSpace is the first chunk with space, we just need to walk forwards from it.
			// (If that's not, we have bigger problems!)
			OccupancyArrayChunk<T> *nextChunk = chunk->nextChunk;
			s32 nextChunkIndex = array->firstChunkWithSpaceIndex + 1;
			while (nextChunk->count == array->itemsPerChunk)
			{
				// NB: We don't check for a null nextChunk. It should never be null - if it is, something is corrupted
				// and we want to crash here, and the dereference will do that so no need to assert!
				// - Sam, 21/08/2019
				nextChunk = nextChunk->nextChunk;
				nextChunkIndex++;
			}

			array->firstChunkWithSpace = nextChunk;
			array->firstChunkWithSpaceIndex = nextChunkIndex;
		}
	}

	ASSERT_PARANOID(result.value == get(array, result.index));

	return result;
}

template<typename T>
OccupancyArrayChunk<T> *getChunkByIndex(OccupancyArray<T> *array, s32 chunkIndex)
{
	ASSERT(chunkIndex >= 0 && chunkIndex < array->chunkCount); //chunkIndex is out of range!

	OccupancyArrayChunk<T> *chunk = null;

	// Shortcuts for known values
	if (chunkIndex == 0)
	{
		chunk = array->firstChunk;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		chunk = array->lastChunk;
	}
	else if (chunkIndex == array->firstChunkWithSpaceIndex)
	{
		chunk = array->firstChunkWithSpace;
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
void removeIndex(OccupancyArray<T> *array, s32 indexToRemove)
{
	ASSERT(indexToRemove < array->chunkCount * array->itemsPerChunk);

	s32 chunkIndex = indexToRemove / array->itemsPerChunk;
	s32 itemIndex  = indexToRemove % array->itemsPerChunk;

	OccupancyArrayChunk<T> *chunk = getChunkByIndex(array, chunkIndex);

	// Mark it as unoccupied
	ASSERT(chunk->occupancy[itemIndex]);
	unsetBit(&chunk->occupancy, itemIndex);

	// Decrease counts
	chunk->count--;
	array->count--;

	// Update firstChunkWithSpace/Index if the index is lower.
	if (chunkIndex < array->firstChunkWithSpaceIndex)
	{
		array->firstChunkWithSpaceIndex = chunkIndex;
		array->firstChunkWithSpace = chunk;
	}
}

template<typename T>
T *get(OccupancyArray<T> *array, s32 index)
{
	ASSERT(index < array->chunkCount * array->itemsPerChunk);

	T *result = null;

	s32 chunkIndex = index / array->itemsPerChunk;
	s32 itemIndex  = index % array->itemsPerChunk;

	OccupancyArrayChunk<T> *chunk = getChunkByIndex(array, chunkIndex);

	if (chunk != null && chunk->occupancy[itemIndex])
	{
		result = chunk->items + itemIndex;
	}

	return result;
}

template<typename T>
OccupancyArrayIterator<T> iterate(OccupancyArray<T> *array)
{
	OccupancyArrayIterator<T> iterator = {};

	iterator.array = array;
	iterator.chunkIndex = 0;
	iterator.indexInChunk = 0;
	iterator.currentChunk = array->firstChunk;

	// If the table is empty, we can skip some work.
	iterator.isDone = (array->count == 0);

	// If the first entry is unoccupied, we need to skip ahead
	if (!iterator.isDone && (get(&iterator) == null))
	{
		next(&iterator);
	}

	return iterator;
}

template<typename T>
inline bool hasNext(OccupancyArrayIterator<T> *iterator)
{
	return !iterator->isDone;
}

template<typename T>
void next(OccupancyArrayIterator<T> *iterator)
{
	while (!iterator->isDone)
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
				// We're not wrapping, so we're done
				iterator->isDone = true;
			}
		}

		// Only stop iterating if we find an occupied entry
		if (iterator->currentChunk != null && get(iterator) != null)
		{
			break;
		}
	}
}

template<typename T>
inline T *get(OccupancyArrayIterator<T> *iterator)
{
	T *result = null;

	if (iterator->currentChunk->occupancy[iterator->indexInChunk])
	{
		result = iterator->currentChunk->items + iterator->indexInChunk;
	}

	return result;
}

template<typename T>
inline s32 getIndex(OccupancyArrayIterator<T> *iterator)
{
	s32 result = (iterator->chunkIndex * iterator->array->itemsPerChunk) + iterator->indexInChunk;

	return result;
}
