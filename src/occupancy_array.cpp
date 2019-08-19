
template<typename T>
void initOccupancyArray(OccupancyArray<T> *array, MemoryArena *arena, smm itemsPerChunk)
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
		// We need to append a new chunk and then assign it to firstChunkWithSpace/Index
	}

	// getFirstUnsetBitIndex() to find the free slot

	// mark that slot as occupied, update counts, recalculate firstChunkWithSpace/Index

	return result;
}

template<typename T>
T removeIndex(OccupancyArray<T> *array, smm indexToRemove)
{
	// Mark it as unoccupied

	// Decrease counts

	// Update firstChunkWithSpace/Index if the index is lower.
}

template<typename T>
T *get(OccupancyArray<T> *array, smm index)
{
	ASSERT(index < array->chunkCount * array->itemsPerChunk);

	T *result = null;

	smm chunkIndex = index / array->itemsPerChunk;
	smm itemIndex  = index % array->itemsPerChunk;

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
		OccupancyArrayChunk<T> *chunk = array->firstChunk;
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
