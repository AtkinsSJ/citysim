/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Debug/Debug.h>
#include <Util/Basic.h>
#include <Util/DeprecatedPool.h>
#include <Util/Indexed.h>
#include <Util/Log.h>
#include <Util/Memory.h>

/**
 * Variation on Array where we allocate the data in chunks, which are linked-listed together.
 * This means we can allocate them within a memory arena, and have more control there.
 * BUT, it also means the array won't be contiguous after allocating a second chunk.
 */

template<typename T>
struct ArrayChunk : PoolItem {
    s32 count;
    T* items;

    ArrayChunk<T>* prevChunk;
    ArrayChunk<T>* nextChunk;
};

template<typename T>
ArrayChunk<T>* allocateChunk(MemoryArena* arena, s32 itemsPerChunk)
{
    // Rolled into a single allocation
    Blob blob = arena->allocate_blob(sizeof(ArrayChunk<T>) + (sizeof(T) * itemsPerChunk));
    ArrayChunk<T>* newChunk = (ArrayChunk<T>*)blob.data();
    *newChunk = {};
    newChunk->count = 0;
    newChunk->items = (T*)(blob.data() + sizeof(ArrayChunk<T>));

    return newChunk;
}

template<typename T>
struct ArrayChunkPool : DeprecatedPool<ArrayChunk<T>> {
    s32 itemsPerChunk;
};

template<typename T>
struct ChunkedArrayIterator;

template<typename T>
struct ChunkedArray {
    ArrayChunkPool<T>* chunkPool;
    MemoryArena* memoryArena;

    s32 itemsPerChunk;
    s32 chunkCount;
    s32 count;

    ArrayChunk<T>* firstChunk;
    ArrayChunk<T>* lastChunk;

    // Methods
    bool isEmpty() { return count == 0; }
    T* get(s32 index)
    {
        ASSERT(index >= 0 && index < count); // Index out of array bounds!

        T* result = nullptr;

        s32 chunkIndex = index / itemsPerChunk;
        s32 itemIndex = index % itemsPerChunk;

        if (chunkIndex == 0) {
            // Early out!
            result = firstChunk->items + itemIndex;
        } else if (chunkIndex == (chunkCount - 1)) {
            // Early out!
            result = lastChunk->items + itemIndex;
        } else {
            // Walk the chunk chain
            ArrayChunk<T>* chunk = firstChunk;
            while (chunkIndex > 0) {
                chunkIndex--;
                chunk = chunk->nextChunk;
            }

            result = chunk->items + itemIndex;
        }

        return result;
    }

    T* append(T item)
    {
        T* result = appendUninitialised();
        *result = item;
        return result;
    }

    T* appendBlank()
    {
        T* result = appendUninitialised();
        *result = {};
        return result;
    }

    void reserve(s32 desiredSize)
    {
        DEBUG_FUNCTION();

        while (((itemsPerChunk * chunkCount) - count) < desiredSize) {
            appendChunk();
        }
    }

    // Marks all the chunks as empty. Returns them to a chunkpool if there is one.
    void clear()
    {
        count = 0;
        for (ArrayChunk<T>* chunk = firstChunk; chunk; chunk = chunk->nextChunk) {
            chunk->count = 0;
        }

        if (chunkPool != nullptr) {
            while (chunkCount > 0) {
                returnLastChunkToPool();
            }
        }
    }

    /**
     * This will iterate through every item in the array, starting at whichever initialIndex
     * you specify (default to the beginning).
     * If wrapAround is true, the iterator will wrap from the end to the beginning so that
     * every item is iterated once. If false, we stop after the last item.
     * Example usage:

        for (auto it = array.iterate(randomBelow(random, array.count), true);
            it.hasNext();
            it.next())
        {
            auto thing = it.get();
            // do stuff with the thing
        }
     */
    ChunkedArrayIterator<T> iterate(s32 initialIndex = 0, bool wrapAround = true, bool goBackwards = false)
    {
        ChunkedArrayIterator<T> iterator = {};

        iterator.array = this;
        iterator.itemsIterated = 0;
        iterator.wrapAround = wrapAround;
        iterator.goBackwards = goBackwards;

        // If the array is empty, we can skip some work.
        iterator.isDone = count == 0;

        if (!iterator.isDone) {
            iterator.chunkIndex = initialIndex / itemsPerChunk;
            iterator.currentChunk = getChunkByIndex(iterator.chunkIndex);
            iterator.indexInChunk = initialIndex % itemsPerChunk;
        }

        return iterator;
    }

    inline ChunkedArrayIterator<T> iterateBackwards()
    {
        return iterate(count - 1, false, true);
    }

    // Filter is a function with signature: bool filter(T *value)
    template<typename Filter>
    Optional<Indexed<T*>> find_first(Filter filter)
    {
        for (auto it = iterate(); it.hasNext(); it.next()) {
            T* entry = it.get();
            if (filter(entry))
                return Indexed { entry, it.getIndex() };
        }

        return {};
    }

    bool findAndRemove(T toRemove, bool keepItemOrder = false)
    {
        DEBUG_FUNCTION();

        s32 removed = removeAll([&](T* t) { return *t == toRemove; }, 1, keepItemOrder);

        return removed > 0;
    }

    void removeIndex(s32 indexToRemove, bool keepItemOrder = false)
    {
        DEBUG_FUNCTION();

        if (indexToRemove < 0 || indexToRemove >= count) {
            logError("Attempted to remove non-existent index {0} from a ChunkedArray!"_s, { formatInt(indexToRemove) });
            return;
        }

        ArrayChunk<T>* lastNonEmptyChunk = getLastNonEmptyChunk();

        if (keepItemOrder) {
            if (indexToRemove != (count - 1)) {
                // NB: This copies the item we're about to remove to the end of the array.
                // I guess if Item is large, this could be expensive unnecessarily?
                // - Sam, 8/2/2019
                moveItemKeepingOrder(indexToRemove, count - 1);
            }
        } else {
            s32 chunkIndex = indexToRemove / itemsPerChunk;
            s32 itemIndex = indexToRemove % itemsPerChunk;

            ArrayChunk<T>* chunk = getChunkByIndex(chunkIndex);

            // We don't need to rearrange things if we're removing the last item
            if (indexToRemove != count - 1) {
                // Copy last item to overwrite this one
                chunk->items[itemIndex] = lastNonEmptyChunk->items[lastNonEmptyChunk->count - 1];
            }
        }

        lastNonEmptyChunk->count--;
        count--;

        // Return empty chunks to the chunkpool
        if ((chunkPool != nullptr) && (lastChunk != nullptr) && (lastChunk->count == 0)) {
            returnLastChunkToPool();
        }
    }

    template<typename Filter>
    s32 removeAll(Filter filter, s32 limit = -1, bool keepItemOrder = false)
    {
        DEBUG_FUNCTION();

        s32 removedCount = 0;
        bool limited = (limit != -1);

        for (ArrayChunk<T>* chunk = firstChunk;
            chunk != nullptr;
            chunk = chunk->nextChunk) {
            for (s32 i = 0; i < chunk->count; i++) {
                if (filter(chunk->items + i)) {
                    // FOUND ONE!
                    removeIndex(i, keepItemOrder);
                    removedCount++;

                    if (limited && removedCount >= limit) {
                        break;
                    } else {
                        // We just moved a different element into position i, so make sure we
                        // check that one too!
                        i--;
                    }
                }
            }

            if (limited && removedCount >= limit) {
                break;
            }
        }

        // Return empty chunks to the chunkpool
        if (removedCount && (chunkPool != nullptr)) {
            while ((lastChunk != nullptr) && (lastChunk->count == 0)) {
                returnLastChunkToPool();
            }
        }

        return removedCount;
    }

    // Inserts the item at fromIndex into toIndex, moving other items around as necessary
    void moveItemKeepingOrder(s32 fromIndex, s32 toIndex)
    {
        DEBUG_FUNCTION();

        // Skip if there's nothing to do
        if (fromIndex == toIndex)
            return;

        if (fromIndex < toIndex) {
            // Moving >, so move each item in the range left 1
            s32 chunkIndex = fromIndex / itemsPerChunk;
            s32 itemIndex = fromIndex % itemsPerChunk;
            ArrayChunk<T>* chunk = getChunkByIndex(chunkIndex);

            T movingItem = chunk->items[itemIndex];

            for (s32 currentPosition = fromIndex; currentPosition < toIndex; currentPosition++) {
                T* dest = &chunk->items[itemIndex];

                itemIndex++;
                if (itemIndex >= itemsPerChunk) {
                    chunk = chunk->nextChunk;
                    itemIndex = 0;
                }

                T* src = &chunk->items[itemIndex];

                *dest = *src;
            }

            chunk->items[itemIndex] = movingItem;
        } else {
            // Moving <, so move each item in the range right 1
            s32 chunkIndex = fromIndex / itemsPerChunk;
            s32 itemIndex = fromIndex % itemsPerChunk;
            ArrayChunk<T>* chunk = getChunkByIndex(chunkIndex);

            T movingItem = chunk->items[itemIndex];

            for (s32 currentPosition = fromIndex; currentPosition > toIndex; currentPosition--) {
                T* dest = &chunk->items[itemIndex];
                itemIndex--;
                if (itemIndex < 0) {
                    chunk = chunk->prevChunk;
                    itemIndex = itemsPerChunk - 1;
                }
                T* src = &chunk->items[itemIndex];

                *dest = *src;
            }

            chunk->items[itemIndex] = movingItem;
        }
    }

    template<typename Comparison>
    void sort(Comparison compareElements)
    {
        sortInternal(compareElements, 0, count - 1);
    }

    // "private"
    T* appendUninitialised()
    {
        bool useLastChunk = (count >= itemsPerChunk * (chunkCount - 1));
        if (count >= (itemsPerChunk * chunkCount)) {
            appendChunk();
            useLastChunk = true;
        }

        // Shortcut to the last chunk, because that's what we want 99% of the time!
        ArrayChunk<T>* chunk = nullptr;
        if (useLastChunk) {
            chunk = lastChunk;
        } else {
            chunk = firstChunk;
            s32 indexWithinChunk = count;
            while (indexWithinChunk >= itemsPerChunk) {
                chunk = chunk->nextChunk;
                indexWithinChunk -= itemsPerChunk;
            }
        }

        count++;

        T* result = chunk->items + chunk->count++;

        return result;
    }

    void appendChunk()
    {
        ArrayChunk<T>* newChunk = nullptr;

        // Attempt to get a chunk from the pool if we can
        if (chunkPool != nullptr) {
            newChunk = getItemFromPool(chunkPool);
        } else {
            newChunk = allocateChunk<T>(memoryArena, itemsPerChunk);
        }
        newChunk->prevChunk = lastChunk;
        newChunk->nextChunk = nullptr;

        chunkCount++;
        if (lastChunk != nullptr) {
            lastChunk->nextChunk = newChunk;
        }
        lastChunk = newChunk;
        if (firstChunk == nullptr) {
            firstChunk = newChunk;
        }
    }

    ArrayChunk<T>* getChunkByIndex(s32 chunkIndex)
    {
        ASSERT(chunkIndex >= 0 && chunkIndex < chunkCount); // chunkIndex is out of range!

        ArrayChunk<T>* chunk = nullptr;

        // Shortcuts for known values
        if (chunkIndex == 0) {
            chunk = firstChunk;
        } else if (chunkIndex == (chunkCount - 1)) {
            chunk = lastChunk;
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
    ArrayChunk<T>* getLastNonEmptyChunk()
    {
        ArrayChunk<T>* lastNonEmptyChunk = lastChunk;

        if (count > 0) {
            while (lastNonEmptyChunk->count == 0) {
                lastNonEmptyChunk = lastNonEmptyChunk->prevChunk;
            }
        }

        return lastNonEmptyChunk;
    }

    void returnLastChunkToPool()
    {
        ArrayChunk<T>* chunk = lastChunk;

        ASSERT(chunk->count == 0); // Attempting to return a non-empty chunk to the chunk pool!
        lastChunk = lastChunk->prevChunk;
        if (firstChunk == chunk)
            firstChunk = lastChunk;
        chunkCount--;

        addItemToPool<ArrayChunk<T>>(chunkPool, chunk);
    }

    template<typename Comparison>
    void sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex)
    {
        // Quicksort implementation, copied from sortArrayInternal().
        // This is probably really terrible, because we're not specifically handling the chunked-ness,
        // so yeah. @Speed

        if (lowIndex < highIndex) {
            s32 partitionIndex = 0;
            {
                T* pivot = get(highIndex);
                s32 i = (lowIndex - 1);
                for (s32 j = lowIndex; j < highIndex; j++) {
                    T* itemJ = get(j);
                    if (compareElements(itemJ, pivot)) {
                        i++;
                        T* itemI = get(i);
                        T temp = {};

                        temp = *itemI;
                        *itemI = *itemJ;
                        *itemJ = temp;
                    }
                }

                T* itemIPlus1 = get(i + 1);
                T* itemHighIndex = get(highIndex);
                T temp = {};

                temp = *itemIPlus1;
                *itemIPlus1 = *itemHighIndex;
                *itemHighIndex = temp;

                partitionIndex = i + 1;
            }

            sortInternal(compareElements, lowIndex, partitionIndex - 1);
            sortInternal(compareElements, partitionIndex + 1, highIndex);
        }
    }
};

template<typename T>
void initChunkedArray(ChunkedArray<T>* array, MemoryArena* arena, s32 itemsPerChunk)
{
    array->memoryArena = arena;
    array->chunkPool = nullptr;
    array->itemsPerChunk = itemsPerChunk;
    array->chunkCount = 0;
    array->count = 0;
    array->firstChunk = nullptr;
    array->lastChunk = nullptr;

    array->appendChunk();
}

template<typename T>
void initChunkedArray(ChunkedArray<T>* array, ArrayChunkPool<T>* pool)
{
    array->memoryArena = nullptr;
    array->chunkPool = pool;
    array->itemsPerChunk = pool->itemsPerChunk;
    array->chunkCount = 0;
    array->count = 0;
    array->firstChunk = nullptr;
    array->lastChunk = nullptr;
}

template<typename T>
struct ChunkedArrayIterator {
    ChunkedArray<T>* array;
    bool wrapAround;
    bool goBackwards;

    ArrayChunk<T>* currentChunk;
    s32 chunkIndex;
    s32 indexInChunk;

    // This is a counter for use when we start not at the beginning of the array but want to iterate it ALL.
    // For simplicity, we increment it each time we next(), and when it equals the count, we're done.
    s32 itemsIterated;
    bool isDone;

    // Methods
    bool hasNext() { return !isDone; }
    void next()
    {
        if (isDone)
            return;

        itemsIterated++;
        if (itemsIterated >= array->count) {
            // We've seen each index once
            isDone = true;
        }

        if (goBackwards) {
            indexInChunk--;

            if (indexInChunk < 0) {
                // Prev chunk
                currentChunk = currentChunk->prevChunk;
                chunkIndex--;

                if (currentChunk == nullptr) {
                    if (wrapAround) {
                        // Wrap to the beginning!
                        chunkIndex = array->chunkCount - 1;
                        currentChunk = array->lastChunk;
                        indexInChunk = currentChunk->count - 1;
                    } else {
                        // We're not wrapping, so we're done
                        isDone = true;
                    }
                } else {
                    indexInChunk = currentChunk->count - 1;
                }
            }
        } else {
            indexInChunk++;

            if (indexInChunk >= currentChunk->count) {
                // Next chunk
                chunkIndex++;
                currentChunk = currentChunk->nextChunk;
                indexInChunk = 0;

                if (currentChunk == nullptr) {
                    if (wrapAround) {
                        // Wrap to the beginning!
                        chunkIndex = 0;
                        currentChunk = array->firstChunk;
                    } else {
                        // We're not wrapping, so we're done
                        isDone = true;
                    }
                }
            }
        }

        ASSERT(isDone || indexInChunk >= 0 && indexInChunk < currentChunk->count); // Bounds check
    }

    T* get() { return &currentChunk->items[indexInChunk]; }
    s32 getIndex() { return (chunkIndex * array->itemsPerChunk) + indexInChunk; }
    T getValue() { return currentChunk->items[indexInChunk]; }
};

template<typename T>
void initChunkPool(ArrayChunkPool<T>* pool, MemoryArena* arena, s32 itemsPerChunk)
{
    pool->itemsPerChunk = itemsPerChunk;

    initPool<ArrayChunk<T>>(pool, arena, [](MemoryArena* arena, void* userData) {
    s32 itemsPerChunk = *((s32*)userData);
    return allocateChunk<T>(arena, itemsPerChunk); }, &pool->itemsPerChunk);
}
