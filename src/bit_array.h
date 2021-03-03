#pragma once

struct BitArrayIterator;

struct BitArray
{
	s32 size;
	s32 setBitCount;

	Array<u64> u64s;

	bool operator[](u32 index);

	// TODO: Could keep a record of the highest/lowest set/unset bit indices, rather than calculating them each time!
	// Wouldn't even be that hard, just a min() or max() when a bit changes.

// Methods

	// NB: This only handles iterating through set bits, because that's all we need right now,
	// and I'm not sure iterating through unset bits too is useful.
	// (Because you can just do a regular
	//     for (s32 i=0; i < array.size; i++) { do thing with array[i]; }
	// loop in that case!)
	// - Sam, 18/08/2019
	BitArrayIterator iterateSetBits();
};

void initBitArray(BitArray *array, MemoryArena *arena, s32 size);

// If you want, you can supply the memory directly, in case you want to allocate it with something else.
// Of course, you MUST pass an array of u64s that is big enough!
// IMPORTANT: we assume that the array is set to all 0s!
// To get the size, call calculateBitArrayU64Count() below.
void initBitArray(BitArray *array, s32 size, Array<u64> u64s);
inline s32 calculateBitArrayU64Count(s32 bitCount)
{
	return 1 + ((bitCount-1) / 64);
}

void setBit(BitArray *array, s32 index);
void unsetBit(BitArray *array, s32 index);

void clearBits(BitArray *array);

// Returns a temporary array containing the indices of the set bits from this array
Array<s32> getSetBitIndices(BitArray *array);

s32 getFirstSetBitIndex(BitArray *array);
s32 getFirstUnsetBitIndex(BitArray *array);
s32 getFirstMatchingBitIndex(BitArray *array, bool set);

struct BitArrayIterator
{
	BitArray *array;
	s32 currentIndex;
	bool isDone;

// Methods
	bool hasNext();
	void next();
	s32 getIndex();
	bool getValue();
};

BitArrayIterator iterateSetBits(BitArray *array) { return array->iterateSetBits(); }
void next(BitArrayIterator *iterator) { iterator->next(); }
bool hasNext(BitArrayIterator *iterator) { return iterator->hasNext(); }
s32 getIndex(BitArrayIterator *iterator) { return iterator->getIndex(); }
bool getValue(BitArrayIterator *iterator) { return iterator->getValue(); }
