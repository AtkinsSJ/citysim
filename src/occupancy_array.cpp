 
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
Indexed<T*> OccupancyArray<T>::append()
{
	Indexed<T*> result = {};

	if (firstChunkWithSpace == null)
	{
		// Append a new chunk

		// @Copypasta ArrayChunk::allocateChunk(). We probably want to move this logic into
		// an "allocate a struct with its data array" function somehow? That's not easy in C++ though.
		// Actually, this one has two extra parts, so that's even more complicated...
		// - Sam, 21/08/2019
		smm structSize = sizeof(OccupancyArrayChunk<T>);
		smm arraySize = sizeof(T) * itemsPerChunk;
		s32 occupancyArrayCount = BitArray::calculateU64Count(itemsPerChunk);
		smm occupancyArraySize = occupancyArrayCount * sizeof(u64);

		Blob blob = allocateBlob(memoryArena, structSize + arraySize + occupancyArraySize);
		OccupancyArrayChunk<T> *newChunk = (OccupancyArrayChunk<T> *)blob.memory;
		*newChunk = {};
		newChunk->items = (T *)(blob.memory + structSize);
		initBitArray(&newChunk->occupancy, itemsPerChunk, makeArray(occupancyArrayCount, (u64 *)(blob.memory + structSize + arraySize)));

		chunkCount++;

		// Attach the chunk to the end, but also make it the "firstChunkWithSpace"
		if (firstChunk == null) firstChunk = newChunk;

		if (lastChunk != null)
		{
			lastChunk->nextChunk = newChunk;
			newChunk->prevChunk = lastChunk;
		}
		lastChunk = newChunk;

		firstChunkWithSpace = newChunk;
		firstChunkWithSpaceIndex = chunkCount - 1;
	}

	OccupancyArrayChunk<T> *chunk = firstChunkWithSpace;

	// getFirstUnsetBitIndex() to find the free slot
	s32 indexInChunk = chunk->occupancy.getFirstUnsetBitIndex();
	ASSERT(indexInChunk >= 0 && indexInChunk < itemsPerChunk);

	// mark that slot as occupied
	chunk->occupancy.setBit(indexInChunk);
	result.index = indexInChunk + (firstChunkWithSpaceIndex * itemsPerChunk);
	result.value = chunk->items + indexInChunk;

	// Make sure the "new" element is actually new and blank and fresh
	*result.value = {};

	// update counts
	count++;

	if (chunk->occupancy.setBitCount == itemsPerChunk)
	{
		// recalculate firstChunkWithSpace/Index
		if (count == itemsPerChunk * chunkCount)
		{
			// We're full! So just set the firstChunkWithSpace to null
			firstChunkWithSpace = null;
			firstChunkWithSpaceIndex = -1;
		}
		else
		{
			// There's a chunk with space, we just have to find it
			// Assuming that firstChunkWithSpace is the first chunk with space, we just need to walk forwards from it.
			// (If that's not, we have bigger problems!)
			OccupancyArrayChunk<T> *nextChunk = chunk->nextChunk;
			s32 nextChunkIndex = firstChunkWithSpaceIndex + 1;
			while (nextChunk->occupancy.setBitCount == itemsPerChunk)
			{
				// NB: We don't check for a null nextChunk. It should never be null - if it is, something is corrupted
				// and we want to crash here, and the dereference will do that so no need to assert!
				// - Sam, 21/08/2019
				nextChunk = nextChunk->nextChunk;
				nextChunkIndex++;
			}

			firstChunkWithSpace = nextChunk;
			firstChunkWithSpaceIndex = nextChunkIndex;
		}
	}

	ASSERT(result.value == get(result.index));

	return result;
}

template<typename T>
void OccupancyArray<T>::removeIndex(s32 indexToRemove)
{
	ASSERT(indexToRemove < chunkCount * itemsPerChunk);

	s32 chunkIndex = indexToRemove / itemsPerChunk;
	s32 itemIndex  = indexToRemove % itemsPerChunk;

	OccupancyArrayChunk<T> *chunk = getChunkByIndex(chunkIndex);

	// Mark it as unoccupied
	ASSERT(chunk->occupancy[itemIndex]);
	chunk->occupancy.unsetBit(itemIndex);

	// Decrease counts
	count--;

	// Update firstChunkWithSpace/Index if the index is lower.
	if (chunkIndex < firstChunkWithSpaceIndex)
	{
		firstChunkWithSpaceIndex = chunkIndex;
		firstChunkWithSpace = chunk;
	}
}

template<typename T>
T *OccupancyArray<T>::get(s32 index)
{
	ASSERT(index < chunkCount * itemsPerChunk);

	T *result = null;

	s32 chunkIndex = index / itemsPerChunk;
	s32 itemIndex  = index % itemsPerChunk;

	OccupancyArrayChunk<T> *chunk = getChunkByIndex(chunkIndex);

	if (chunk != null && chunk->occupancy[itemIndex])
	{
		result = chunk->items + itemIndex;
	}

	return result;
}

template<typename T>
OccupancyArrayIterator<T> OccupancyArray<T>::iterate()
{
	OccupancyArrayIterator<T> iterator = {};

	iterator.array = this;
	iterator.chunkIndex = 0;
	iterator.indexInChunk = 0;
	iterator.currentChunk = firstChunk;

	// If the table is empty, we can skip some work.
	iterator.isDone = (count == 0);

	// If the first entry is unoccupied, we need to skip ahead
	if (!iterator.isDone && (iterator.get() == null))
	{
		iterator.next();
	}

	return iterator;
}

template<typename T>
OccupancyArrayChunk<T> *OccupancyArray<T>::getChunkByIndex(s32 chunkIndex)
{
	ASSERT(chunkIndex >= 0 && chunkIndex < chunkCount); //chunkIndex is out of range!

	OccupancyArrayChunk<T> *chunk = null;

	// Shortcuts for known values
	if (chunkIndex == 0)
	{
		chunk = firstChunk;
	}
	else if (chunkIndex == (chunkCount - 1))
	{
		chunk = lastChunk;
	}
	else if (chunkIndex == firstChunkWithSpaceIndex)
	{
		chunk = firstChunkWithSpace;
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
inline bool OccupancyArrayIterator<T>::hasNext()
{
	return !isDone;
}

template<typename T>
void OccupancyArrayIterator<T>::next()
{
	while (!isDone)
	{
		indexInChunk++;

		if (indexInChunk >= array->itemsPerChunk)
		{
			// Next chunk
			chunkIndex++;
			currentChunk = currentChunk->nextChunk;
			indexInChunk = 0;

			if (currentChunk == null)
			{
				// We're not wrapping, so we're done
				isDone = true;
			}
		}

		// Only stop iterating if we find an occupied entry
		if (currentChunk != null && get() != null)
		{
			break;
		}
	}
}

template<typename T>
inline T *OccupancyArrayIterator<T>::get()
{
	T *result = null;

	if (currentChunk->occupancy[indexInChunk])
	{
		result = currentChunk->items + indexInChunk;
	}

	return result;
}

template<typename T>
inline s32 OccupancyArrayIterator<T>::getIndex()
{
	s32 result = (chunkIndex * array->itemsPerChunk) + indexInChunk;

	return result;
}
