/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/BitArray.h>
#include <Util/Indexed.h>
#include <Util/MemoryArena.h>

template<typename T>
struct OccupancyArrayChunk {
    T* items;
    BitArray occupancy;

    OccupancyArrayChunk<T>* prevChunk;
    OccupancyArrayChunk<T>* nextChunk;
};

template<typename T>
struct OccupancyArrayIterator;

// A chunked array that keeps track of which slots are empty, and attempts to fill them.
// (The regular ChunkedArray just moves the elements to fill empty slots, so the order can change.)
template<typename T>
struct OccupancyArray {
    OccupancyArray() = default; // FIXME: Temporary
    OccupancyArray(MemoryArena& arena, s32 itemsPerChunk)
        : memoryArena(&arena)
        , itemsPerChunk(itemsPerChunk)
    {
    }

    MemoryArena* memoryArena;

    s32 itemsPerChunk;
    s32 chunkCount { 0 };
    s32 count { 0 };

    OccupancyArrayChunk<T>* firstChunk { nullptr };
    OccupancyArrayChunk<T>* lastChunk { nullptr };

    s32 firstChunkWithSpaceIndex { -1 };
    OccupancyArrayChunk<T>* firstChunkWithSpace { nullptr };

    // Methods
    Indexed<T> append()
    {
        if (firstChunkWithSpace == nullptr) {
            // Append a new chunk
            smm arraySize = sizeof(T) * itemsPerChunk;
            s32 occupancyArrayCount = BitArray::calculate_u64_count(itemsPerChunk);
            smm occupancyArraySize = occupancyArrayCount * sizeof(u64);

            auto [new_chunk, items] = memoryArena->allocate_with_data<OccupancyArrayChunk<T>>(arraySize + occupancyArraySize);
            new_chunk.items = reinterpret_cast<T*>(items.raw_data());
            new_chunk.occupancy = BitArray::from_memory(itemsPerChunk, makeArray(occupancyArrayCount, reinterpret_cast<u64*>(items.raw_data() + arraySize), occupancyArrayCount));

            chunkCount++;

            // Attach the chunk to the end, but also make it the "firstChunkWithSpace"
            if (firstChunk == nullptr)
                firstChunk = &new_chunk;

            if (lastChunk != nullptr) {
                lastChunk->nextChunk = &new_chunk;
                new_chunk.prevChunk = lastChunk;
            }
            lastChunk = &new_chunk;

            firstChunkWithSpace = &new_chunk;
            firstChunkWithSpaceIndex = chunkCount - 1;
        }

        OccupancyArrayChunk<T>* chunk = firstChunkWithSpace;

        // get_first_unset_bit_index() to find the free slot
        s32 indexInChunk = chunk->occupancy.get_first_unset_bit_index();
        ASSERT(indexInChunk >= 0 && indexInChunk < itemsPerChunk);

        // mark that slot as occupied
        chunk->occupancy.set_bit(indexInChunk);
        Indexed<T> result {
            indexInChunk + (firstChunkWithSpaceIndex * itemsPerChunk),
            chunk->items[indexInChunk]
        };

        // Make sure the "new" element is actually new and blank and fresh
        // FIXME: This is weird.
        result.value() = {};

        // update counts
        count++;

        if (chunk->occupancy.is_all_set()) {
            // recalculate firstChunkWithSpace/Index
            if (count == itemsPerChunk * chunkCount) {
                // We're full! So just set the firstChunkWithSpace to null
                firstChunkWithSpace = nullptr;
                firstChunkWithSpaceIndex = -1;
            } else {
                // There's a chunk with space, we just have to find it
                // Assuming that firstChunkWithSpace is the first chunk with space, we just need to walk forwards from it.
                // (If that's not, we have bigger problems!)
                OccupancyArrayChunk<T>* nextChunk = chunk->nextChunk;
                s32 nextChunkIndex = firstChunkWithSpaceIndex + 1;
                while (nextChunk->occupancy.is_all_set()) {
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

        ASSERT(&result.value() == get(result.index()));

        return result;
    }

    template<typename... Args>
    Indexed<T> empend(Args&&... args)
    {
        // FIXME: Using append() is a stop-gap; generalize both with an internal method that gets the memory for a new item.
        auto result = append();
        new (&result.value()) T(forward<Args>(args)...);
        return result;
    }

    T* get(s32 index)
    {
        ASSERT(index < chunkCount * itemsPerChunk);

        T* result = nullptr;

        s32 chunkIndex = index / itemsPerChunk;
        s32 itemIndex = index % itemsPerChunk;

        OccupancyArrayChunk<T>* chunk = getChunkByIndex(chunkIndex);

        if (chunk != nullptr && chunk->occupancy[itemIndex]) {
            result = chunk->items + itemIndex;
        }

        return result;
    }

    void removeIndex(s32 indexToRemove)
    {
        ASSERT(indexToRemove < chunkCount * itemsPerChunk);

        s32 chunkIndex = indexToRemove / itemsPerChunk;
        s32 itemIndex = indexToRemove % itemsPerChunk;

        OccupancyArrayChunk<T>* chunk = getChunkByIndex(chunkIndex);

        // Mark it as unoccupied
        ASSERT(chunk->occupancy[itemIndex]);
        chunk->occupancy.unset_bit(itemIndex);

        // Decrease counts
        count--;

        // Update firstChunkWithSpace/Index if the index is lower.
        if (chunkIndex < firstChunkWithSpaceIndex) {
            firstChunkWithSpaceIndex = chunkIndex;
            firstChunkWithSpace = chunk;
        }
    }

    OccupancyArrayIterator<T> iterate()
    {
        OccupancyArrayIterator<T> iterator = {};

        iterator.array = this;
        iterator.chunkIndex = 0;
        iterator.indexInChunk = 0;
        iterator.currentChunk = firstChunk;

        // If the table is empty, we can skip some work.
        iterator.isDone = (count == 0);

        // If the first entry is unoccupied, we need to skip ahead
        if (!iterator.isDone && (iterator.get() == nullptr)) {
            iterator.next();
        }

        return iterator;
    }

    OccupancyArrayIterator<T> iterate() const
    {
        return const_cast<OccupancyArray*>(this)->iterate();
    }

    OccupancyArrayChunk<T>* getChunkByIndex(s32 chunkIndex)
    {
        ASSERT(chunkIndex >= 0 && chunkIndex < chunkCount); // chunkIndex is out of range!

        OccupancyArrayChunk<T>* chunk = nullptr;

        // Shortcuts for known values
        if (chunkIndex == 0) {
            chunk = firstChunk;
        } else if (chunkIndex == (chunkCount - 1)) {
            chunk = lastChunk;
        } else if (chunkIndex == firstChunkWithSpaceIndex) {
            chunk = firstChunkWithSpace;
        } else {
            // Walk the chunk chain
            chunk = firstChunk;
            while (chunkIndex > 0) {
                chunkIndex--;
                chunk = chunk->nextChunk;
            }
        }

        return chunk;
    }
};

template<typename T>
struct OccupancyArrayIterator {
    OccupancyArray<T>* array;

    OccupancyArrayChunk<T>* currentChunk;
    s32 chunkIndex;
    s32 indexInChunk;

    bool isDone;

    // Methods
    bool hasNext() { return !isDone; }

    void next()
    {
        while (!isDone) {
            indexInChunk++;

            if (indexInChunk >= array->itemsPerChunk) {
                // Next chunk
                chunkIndex++;
                currentChunk = currentChunk->nextChunk;
                indexInChunk = 0;

                if (currentChunk == nullptr) {
                    // We're not wrapping, so we're done
                    isDone = true;
                }
            }

            // Only stop iterating if we find an occupied entry
            if (currentChunk != nullptr && get() != nullptr) {
                break;
            }
        }
    }

    T* get()
    {
        T* result = nullptr;

        if (currentChunk->occupancy[indexInChunk]) {
            result = currentChunk->items + indexInChunk;
        }

        return result;
    }

    s32 getIndex()
    {
        s32 result = (chunkIndex * array->itemsPerChunk) + indexInChunk;

        return result;
    }
};
