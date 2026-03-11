/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Basic.h>
#include <Util/Forward.h>

class BitArray {
public:
    s32 size;
    s32 setBitCount;

    Array<u64> u64s;

    bool operator[](u32 index) const;

    // TODO: Could keep a record of the highest/lowest set/unset bit indices, rather than calculating them each time!
    // Wouldn't even be that hard, just a min() or max() when a bit changes.

    // Methods

    static constexpr s32 calculate_u64_count(s32 bitCount)
    {
        return 1 + ((bitCount - 1) / 64);
    }

    void set_bit(s32 index);
    void unset_bit(s32 index);
    void set_all();
    void unset_all();

    // Returns a temporary array containing the indices of the set bits from this array
    Array<s32> get_set_bit_indices() const;
    s32 get_first_set_bit_index() const;
    s32 get_first_unset_bit_index() const;
    s32 get_first_matching_bit_index(bool set) const;

    // NB: This only handles iterating through set bits, because that's all we need right now,
    // and I'm not sure iterating through unset bits too is useful.
    // (Because you can just do a regular
    //     for (s32 i=0; i < array.size; i++) { do thing with array[i]; }
    // loop in that case!)
    // - Sam, 18/08/2019
    BitArrayIterator iterate_set_bits() const;
};

void initBitArray(BitArray* array, MemoryArena* arena, s32 size);

// If you want, you can supply the memory directly, in case you want to allocate it with something else.
// Of course, you MUST pass an array of u64s that is big enough!
// IMPORTANT: we assume that the array is set to all 0s!
// To get the size, call calculateU64Count() below.
void initBitArray(BitArray* array, s32 size, Array<u64> u64s);

class BitArrayIterator {
public:
    explicit BitArrayIterator(BitArray const&);

    bool has_next() const;
    void next();
    s32 get_index() const;
    bool get_value() const;

private:
    BitArray const& m_array;
    s32 m_current_index;
    bool m_is_done;
};
