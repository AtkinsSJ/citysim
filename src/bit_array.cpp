
void initBitArray(BitArray *array, MemoryArena *arena, s32 size)
{
	array->size = size;
	array->setBitCount = 0;
	array->u64s = allocateArray<u64>(arena, calculateBitArrayU64Count(size));
}

void initBitArray(BitArray *array, s32 size, Array<u64> u64s)
{
	ASSERT(u64s.count * 64 >= size);

	array->size = size;
	array->setBitCount = 0;
	array->u64s = u64s;
}

inline bool BitArray::operator[](u32 index)
{
	bool result = false;

	// NB: Check and assert done this way so that in debug builds, we assert, but
	// in release builds without asserts, we just return false for non-existent bits.
	if (index >= (u32)this->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		u32 fieldIndex = index >> 6;
		u32 bitIndex = index & 63;
		u64 mask = ((u64)1 << bitIndex);
		result = (this->u64s[fieldIndex] & mask) != 0;
	}

	return result;
}

void setBit(BitArray *array, s32 index)
{
	// NB: Check and assert done this way so that in debug builds, we assert, but
	// in release builds without asserts, we just do nothing for non-existent bits.
	if (index >= array->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		u32 fieldIndex = index >> 6;
		u32 bitIndex = index & 63;
		u64 mask = ((u64)1 << bitIndex);

		bool wasSet = (array->u64s[fieldIndex] & mask) != 0;

		if (!wasSet)
		{
			array->u64s[fieldIndex] |= mask;
			array->setBitCount++;
		}
	}
}

void unsetBit(BitArray *array, s32 index)
{
	// NB: Check and assert done this way so that in debug builds, we assert, but
	// in release builds without asserts, we just do nothing for non-existent bits. 
	if (index >= array->size || index < 0)
	{
		ASSERT(false);
	}
	else
	{
		u32 fieldIndex = index >> 6;
		u32 bitIndex = index & 63;
		u64 mask = ((u64)1 << bitIndex);

		bool wasSet = (array->u64s[fieldIndex] & mask) != 0;

		if (wasSet)
		{
			array->u64s[fieldIndex] &= ~mask;
			array->setBitCount--;
		}
	}
}

void clearBits(BitArray *array)
{
	array->setBitCount = 0;

	for (s32 i = 0; i < array->u64s.count; i++)
	{
		array->u64s[i] = 0;
	}
}

Array<s32> getSetBitIndices(BitArray *array)
{
	Array<s32> result = allocateArray<s32>(tempArena, array->setBitCount);

	s32 pos = 0;

	for (auto it = iterateSetBits(array); hasNext(&it); next(&it))
	{
		result[pos++] = getIndex(&it);
	}

	return result;
}

inline s32 getFirstSetBitIndex(BitArray *array)
{
	return getFirstMatchingBitIndex(array, true);
}

inline s32 getFirstUnsetBitIndex(BitArray *array)
{
	return getFirstMatchingBitIndex(array, false);
}

s32 getFirstMatchingBitIndex(BitArray *array, bool set)
{
	s32 result = -1;

	if (array->setBitCount != array->size)
	{
		for (s32 index = 0; index < array->size; index++)
		{
			if ((*array)[index] == set)
			{
				result = index;
				break;
			}
		}
	}

	return result;
}

BitArrayIterator BitArray::iterateSetBits()
{
	BitArrayIterator iterator = {};

	iterator.array = this;
	iterator.currentIndex = 0;

	// If bitfield is empty, we can skip some work
	iterator.isDone = (setBitCount == 0);

	// If the first bit is unset, we need to skip ahead
	if (!iterator.isDone && !getValue(&iterator))
	{
		next(&iterator);
	}

	return iterator;
}

void BitArrayIterator::next()
{
	while (!isDone)
	{
		currentIndex++;

		if (currentIndex >= array->size)
		{
			isDone = true;
		}
		else
		{
			// Only stop iterating if we find a set bit
			if (getValue()) break;
		}
	}
}

inline bool BitArrayIterator::hasNext()
{
	return !isDone;
}

inline s32 BitArrayIterator::getIndex()
{
	return currentIndex;
}

inline bool BitArrayIterator::getValue()
{
	return (*array)[currentIndex];
}
