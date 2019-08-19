
void initBitArray(BitArray *array, MemoryArena *arena, s32 size)
{
	array->size = size;
	array->setBitCount = 0;

	// I can't think of a good name for this, but it's how many u64s we need
	s32 chunkCount = 1 + (size / 64);
	array->chunks = allocateArray<u64>(arena, chunkCount);
}

inline bool BitArray::operator[](s32 index)
{
	bool result = false;

	if (index >= this->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		// @Speed: Could do these with bitshifts, once I know it's working right
		s32 fieldIndex = index / 64;
		u64 bitIndex = index % 64;

		u64 mask = ((u64)1 << bitIndex);
		result = (this->chunks[fieldIndex] & mask) != 0;
	}

	return result;
}

void setBit(BitArray *array, s32 index, bool value)
{
	if (index >= array->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		// @Speed: Could do these with bitshifts, once I know it's working right
		s32 fieldIndex = index / 64;
		u64 bitIndex = index % 64;
		
		u64 mask = ((u64)1 << bitIndex);

		bool wasSet = (array->chunks[fieldIndex] & mask) != 0;

		if (wasSet != value)
		{
			if (value)
			{
				array->chunks[fieldIndex] |= mask;
				array->setBitCount++;
			}
			else
			{
				array->chunks[fieldIndex] &= ~mask;
				array->setBitCount--;
			}
		}
	}
}

void clearBits(BitArray *array)
{
	array->setBitCount = 0;

	for (s32 i = 0; i < array->chunks.count; i++)
	{
		array->chunks[i] = 0;
	}
}

Array<s32> getSetBitIndices(BitArray *array)
{
	Array<s32> result = allocateArray<s32>(tempArena, array->setBitCount);

	s32 pos = 0;

	for (auto it = iterateSetBits(array); !it.isDone; next(&it))
	{
		result[pos++] = getIndex(&it);
	}

	return result;
}

s32 getFirstSetBitIndex(BitArray *array)
{
	// TODO: use iterator for speed?
	s32 result = -1;

	if (array->setBitCount > 0)
	{
		for (s32 index = 0; index < array->size; index++)
		{
			if ((*array)[index])
			{
				result = index;
				break;
			}
		}
	}

	return result;
}

s32 getFirstUnsetBitIndex(BitArray *array)
{
	// TODO: use iterator for speed?
	s32 result = -1;

	if (array->setBitCount > 0)
	{
		for (s32 index = 0; index < array->size; index++)
		{
			if (!(*array)[index])
			{
				result = index;
				break;
			}
		}
	}

	return result;
}

BitArrayIterator iterateSetBits(BitArray *array)
{
	BitArrayIterator iterator = {};

	iterator.array = array;
	iterator.currentIndex = 0;

	// If bitfield is empty, we can skip some work
	iterator.isDone = (array->setBitCount == 0);

	// If the first bit is unset, we need to skip ahead
	if (!iterator.isDone && !getValue(&iterator))
	{
		next(&iterator);
	}

	return iterator;
}

void next(BitArrayIterator *iterator)
{
	while (!iterator->isDone)
	{
		iterator->currentIndex++;

		if (iterator->currentIndex >= iterator->array->size)
		{
			iterator->isDone = true;
		}
		else
		{
			// Only stop iterating if we find a set bit
			if (getValue(iterator)) break;
		}
	}
}

inline s32 getIndex(BitArrayIterator *iterator)
{
	return iterator->currentIndex;
}

inline bool getValue(BitArrayIterator *iterator)
{
	return (*iterator->array)[iterator->currentIndex];
}
