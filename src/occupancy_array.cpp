
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
OccupancyArrayItem<T> append(OccupancyArray<T> *array)
{
	OccupancyArrayItem<T> result = {};

	if (array->firstChunkWithSpace == null)
	{
		// Append a new chunk

		// @Copypasta ArrayChunk::allocateChunk(). We probably want to move this logic into
		// an "allocate a struct with its data array" function somehow? That's not easy in C++ though.
		// - Sam, 21/08/2019
		Blob blob = allocateBlob(array->memoryArena, sizeof(OccupancyArrayChunk<T>) + (sizeof(T) * array->itemsPerChunk));
		OccupancyArrayChunk<T> *newChunk = (OccupancyArrayChunk<T> *)blob.memory;
		*newChunk = {};
		newChunk->count = 0;
		newChunk->items = (T *)(blob.memory + sizeof(OccupancyArrayChunk<T>));
		initBitArray(&newChunk->occupancy, array->memoryArena, array->itemsPerChunk); // TODO: Roll allocation into above?

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
	setBit(&chunk->occupancy, indexInChunk, true);
	result.index = indexInChunk + (array->firstChunkWithSpaceIndex * array->itemsPerChunk);
	result.item = chunk->items + indexInChunk;

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

	ASSERT_PARANOID(result.item == get(array, result.index));

	return result;
}

template<typename T>
T removeIndex(OccupancyArray<T> *array, s32 indexToRemove)
{
	// Mark it as unoccupied

	// Decrease counts

	// Update firstChunkWithSpace/Index if the index is lower.
}

template<typename T>
T *get(OccupancyArray<T> *array, s32 index)
{
	ASSERT(index < array->chunkCount * array->itemsPerChunk);

	T *result = null;

	s32 chunkIndex = index / array->itemsPerChunk;
	s32 itemIndex  = index % array->itemsPerChunk;

	OccupancyArrayChunk<T> *chunk = null;

	if (chunkIndex == 0)
	{
		// Early out!
		chunk = array->firstChunk;
	}
	else if (chunkIndex == (array->chunkCount - 1))
	{
		// Early out!
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

	if (chunk != null && chunk->occupancy[itemIndex])
	{
		result = chunk->items + itemIndex;
	}

	return result;
}
