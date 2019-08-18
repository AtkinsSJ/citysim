#pragma once

struct BitArray
{
	s32 size;
	s32 setBitsCount;

	Array<u64> chunks;

	bool operator[](s32 index);
};

void initBitArray(BitArray *array, MemoryArena *arena, s32 size);

void setBit(BitArray *array, s32 index, bool value);

void clearBits(BitArray *array);

struct BitArrayIterator
{
	BitArray *array;
	s32 currentIndex;
	bool isDone;
};

// NB: This only handles iterating through set bits, because that's all we need right now,
// and I'm not sure iterating through unset bits too is useful.
// (Because you can just do a regular
//     for (s32 i=0; i < array.size; i++) { do thing with array[i]; }
// loop in that case!)
// - Sam, 18/08/2019
BitArrayIterator iterateSetBits(BitArray *array);
void next(BitArrayIterator *iterator);
s32 getIndex(BitArrayIterator *iterator);
bool getValue(BitArrayIterator *iterator);
