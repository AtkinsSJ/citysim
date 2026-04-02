/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BitArray.h"
#include <Util/Assert.h>
#include <Util/MemoryArena.h>

BitArray::BitArray(MemoryArena& arena, s32 size)
    : m_size(size)
    , m_data(arena.allocate_array<u64>(calculate_u64_count(size), true))
{
}

BitArray BitArray::from_memory(s32 size, Array<u64> u64s)
{
    ASSERT(u64s.count * 64 >= size);
    return BitArray { size, u64s };
}

BitArray::BitArray(s32 size, Array<u64> u64s)
    : m_size(size)
    , m_data(u64s)
{
}

bool BitArray::operator[](u32 index) const
{
    bool result = false;

    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just return false for non-existent bits.
    if (index >= (u32)this->m_size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);
        result = (this->m_data[fieldIndex] & mask) != 0;
    }

    return result;
}

void BitArray::set_bit(s32 index)
{
    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just do nothing for non-existent bits.
    if (index >= m_size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);

        bool wasSet = (m_data[fieldIndex] & mask) != 0;

        if (!wasSet) {
            m_data[fieldIndex] |= mask;
            m_set_bit_count++;
        }
    }
}

void BitArray::unset_bit(s32 index)
{
    // NB: Check and assert done this way so that in debug builds, we assert, but
    // in release builds without asserts, we just do nothing for non-existent bits.
    if (index >= m_size || index < 0) {
        ASSERT(false);
    } else {
        u32 fieldIndex = index >> 6;
        u32 bitIndex = index & 63;
        u64 mask = ((u64)1 << bitIndex);

        bool wasSet = (m_data[fieldIndex] & mask) != 0;

        if (wasSet) {
            m_data[fieldIndex] &= ~mask;
            m_set_bit_count--;
        }
    }
}

void BitArray::set_all()
{
    m_set_bit_count = m_size;

    // FIXME: memset?
    for (auto& it : m_data) {
        it = ~0;
    }
}

void BitArray::unset_all()
{
    m_set_bit_count = 0;

    // FIXME: memset?
    for (auto& it : m_data) {
        it = 0;
    }
}

Array<s32> BitArray::get_set_bit_indices() const
{
    Array<s32> result = temp_arena().allocate_array<s32>(m_set_bit_count, false);

    for (auto it = iterate_set_bits(); it.has_next(); it.next()) {
        result.append(it.get_index());
    }

    return result;
}

s32 BitArray::get_first_set_bit_index() const
{
    return get_first_matching_bit_index(true);
}

s32 BitArray::get_first_unset_bit_index() const
{
    return get_first_matching_bit_index(false);
}

s32 BitArray::get_first_matching_bit_index(bool set) const
{
    s32 result = -1;

    if (m_set_bit_count != m_size) {
        for (s32 index = 0; index < m_size; index++) {
            if ((*this)[index] == set) {
                result = index;
                break;
            }
        }
    }

    return result;
}

BitArrayIterator BitArray::iterate_set_bits() const
{
    return BitArrayIterator(*this);
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

BitArrayIterator::BitArrayIterator(BitArray const& array)
    : m_array(array)
    , m_current_index(0)
    , m_is_done(array.is_all_unset())
{
    // If the first bit is unset, we need to skip ahead
    if (has_next() && !get_value())
        next();
}

void BitArrayIterator::next()
{
    while (!m_is_done) {
        m_current_index++;

        if (m_current_index >= m_array.size()) {
            m_is_done = true;
        } else {
            // Only stop iterating if we find a set bit
            if (get_value())
                break;
        }
    }
}

bool BitArrayIterator::has_next() const
{
    return !m_is_done;
}

s32 BitArrayIterator::get_index() const
{
    return m_current_index;
}

bool BitArrayIterator::get_value() const
{
    return m_array[m_current_index];
}
