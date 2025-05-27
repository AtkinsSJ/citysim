/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BitArray.h"
#include "../debug.h"
#include "Assert.h"

void initBitArray(BitArray* array, MemoryArena* arena, s32 size)
{
    array->size = size;
    array->setBitCount = 0;
    array->u64s = allocateArray<u64>(arena, BitArray::calculateU64Count(size), true);
}

void initBitArray(BitArray* array, s32 size, Array<u64> u64s)
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
    if (index >= (u32)this->size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);
        result = (this->u64s[fieldIndex] & mask) != 0;
    }

    return result;
}

void BitArray::setBit(s32 index)
{
    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just do nothing for non-existent bits.
    if (index >= size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);

        bool wasSet = (u64s[fieldIndex] & mask) != 0;

        if (!wasSet) {
            u64s[fieldIndex] |= mask;
            setBitCount++;
        }
    }
}

void BitArray::unsetBit(s32 index)
{
    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just do nothing for non-existent bits.
    if (index >= size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);

        bool wasSet = (u64s[fieldIndex] & mask) != 0;

        if (wasSet) {
            u64s[fieldIndex] &= ~mask;
            setBitCount--;
        }
    }
}

void BitArray::clearBits()
{
    setBitCount = 0;

    for (s32 i = 0; i < u64s.count; i++) {
        u64s[i] = 0;
    }
}

Array<s32> BitArray::getSetBitIndices()
{
    Array<s32> result = allocateArray<s32>(tempArena, setBitCount, false);

    for (auto it = iterateSetBits(); it.hasNext(); it.next()) {
        result.append(it.getIndex());
    }

    return result;
}

inline s32 BitArray::getFirstSetBitIndex()
{
    return getFirstMatchingBitIndex(true);
}

inline s32 BitArray::getFirstUnsetBitIndex()
{
    return getFirstMatchingBitIndex(false);
}

s32 BitArray::getFirstMatchingBitIndex(bool set)
{
    s32 result = -1;

    if (setBitCount != size) {
        for (s32 index = 0; index < size; index++) {
            if ((*this)[index] == set) {
                result = index;
                break;
            }
        }
    }

    return result;
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

BitArrayIterator BitArray::iterateSetBits()
{
    BitArrayIterator iterator = {};

    iterator.array = this;
    iterator.currentIndex = 0;

    // If bitfield is empty, we can skip some work
    iterator.isDone = (setBitCount == 0);

    // If the first bit is unset, we need to skip ahead
    if (!iterator.isDone && !iterator.getValue()) {
        iterator.next();
    }

    return iterator;
}

void BitArrayIterator::next()
{
    while (!isDone) {
        currentIndex++;

        if (currentIndex >= array->size) {
            isDone = true;
        } else {
            // Only stop iterating if we find a set bit
            if (getValue())
                break;
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
