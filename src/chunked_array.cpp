#pragma once

//////////////////////////////////////////////////
// POOL STUFF                                   //
//////////////////////////////////////////////////

template<typename T>
void initChunkPool(ArrayChunkPool<T>* pool, MemoryArena* arena, s32 itemsPerChunk)
{
    pool->itemsPerChunk = itemsPerChunk;

    initPool<ArrayChunk<T>>(pool, arena, [](MemoryArena* arena, void* userData) {
		s32 itemsPerChunk = *((s32*)userData);
		return allocateChunk<T>(arena, itemsPerChunk); }, &pool->itemsPerChunk);
}

//////////////////////////////////////////////////
// CHUNKED ARRAY STUFF                          //
//////////////////////////////////////////////////

template<typename T>
void initChunkedArray(ChunkedArray<T>* array, MemoryArena* arena, s32 itemsPerChunk)
{
    array->memoryArena = arena;
    array->chunkPool = null;
    array->itemsPerChunk = itemsPerChunk;
    array->chunkCount = 0;
    array->count = 0;
    array->firstChunk = null;
    array->lastChunk = null;

    array->appendChunk();
}

template<typename T>
void initChunkedArray(ChunkedArray<T>* array, ArrayChunkPool<T>* pool)
{
    array->memoryArena = null;
    array->chunkPool = pool;
    array->itemsPerChunk = pool->itemsPerChunk;
    array->chunkCount = 0;
    array->count = 0;
    array->firstChunk = null;
    array->lastChunk = null;
}

template<typename T>
ArrayChunk<T>* allocateChunk(MemoryArena* arena, s32 itemsPerChunk)
{
    // Rolled into a single allocation
    Blob blob = allocateBlob(arena, sizeof(ArrayChunk<T>) + (sizeof(T) * itemsPerChunk));
    ArrayChunk<T>* newChunk = (ArrayChunk<T>*)blob.memory;
    *newChunk = {};
    newChunk->count = 0;
    newChunk->items = (T*)(blob.memory + sizeof(ArrayChunk<T>));

    return newChunk;
}

template<typename T>
inline bool ChunkedArray<T>::isEmpty()
{
    return count == 0;
}

template<typename T>
T* ChunkedArray<T>::get(s32 index)
{
    ASSERT(index >= 0 && index < count); // Index out of array bounds!

    T* result = null;

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

template<typename T>
T* ChunkedArray<T>::append(T item)
{
    T* result = appendUninitialised();
    *result = item;
    return result;
}

template<typename T>
T* ChunkedArray<T>::appendBlank()
{
    T* result = appendUninitialised();
    *result = {};
    return result;
}

template<typename T>
void ChunkedArray<T>::reserve(s32 desiredSize)
{
    DEBUG_FUNCTION();

    while (((itemsPerChunk * chunkCount) - count) < desiredSize) {
        appendChunk();
    }
}

template<typename T>
void ChunkedArray<T>::clear()
{
    count = 0;
    for (ArrayChunk<T>* chunk = firstChunk; chunk; chunk = chunk->nextChunk) {
        chunk->count = 0;
    }

    if (chunkPool != null) {
        while (chunkCount > 0) {
            returnLastChunkToPool();
        }
    }
}

template<typename T>
ChunkedArrayIterator<T> ChunkedArray<T>::iterate(s32 initialIndex, bool wrapAround, bool goBackwards)
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

template<typename T>
template<typename Filter>
Indexed<T*> ChunkedArray<T>::findFirst(Filter filter)
{
    Indexed<T*> result = makeNullIndexedValue<T*>();

    for (auto it = iterate(); it.hasNext(); it.next()) {
        T* entry = it.get();
        if (filter(entry)) {
            result = makeIndexedValue(entry, it.getIndex());
            break;
        }
    }

    return result;
}

template<typename T>
bool ChunkedArray<T>::findAndRemove(T toRemove, bool keepItemOrder)
{
    DEBUG_FUNCTION();

    s32 removed = removeAll([&](T* t) { return equals(*t, toRemove); }, 1, keepItemOrder);

    return removed > 0;
}

template<typename T>
void ChunkedArray<T>::removeIndex(s32 indexToRemove, bool keepItemOrder)
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
    if ((chunkPool != null) && (lastChunk != null) && (lastChunk->count == 0)) {
        returnLastChunkToPool();
    }
}

template<typename T>
template<typename Filter>
s32 ChunkedArray<T>::removeAll(Filter filter, s32 limit, bool keepItemOrder)
{
    DEBUG_FUNCTION();

    s32 removedCount = 0;
    bool limited = (limit != -1);

    for (ArrayChunk<T>* chunk = firstChunk;
        chunk != null;
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
    if (removedCount && (chunkPool != null)) {
        while ((lastChunk != null) && (lastChunk->count == 0)) {
            returnLastChunkToPool();
        }
    }

    return removedCount;
}

template<typename T>
void ChunkedArray<T>::moveItemKeepingOrder(s32 fromIndex, s32 toIndex)
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

template<typename T>
template<typename Comparison>
void ChunkedArray<T>::sort(Comparison compareElements)
{
    sortInternal(compareElements, 0, count - 1);
}

template<typename T>
T* ChunkedArray<T>::appendUninitialised()
{
    bool useLastChunk = (count >= itemsPerChunk * (chunkCount - 1));
    if (count >= (itemsPerChunk * chunkCount)) {
        appendChunk();
        useLastChunk = true;
    }

    // Shortcut to the last chunk, because that's what we want 99% of the time!
    ArrayChunk<T>* chunk = null;
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

template<typename T>
void ChunkedArray<T>::appendChunk()
{
    ArrayChunk<T>* newChunk = null;

    // Attempt to get a chunk from the pool if we can
    if (chunkPool != null) {
        newChunk = getItemFromPool(chunkPool);
    } else {
        newChunk = allocateChunk<T>(memoryArena, itemsPerChunk);
    }
    newChunk->prevChunk = lastChunk;
    newChunk->nextChunk = null;

    chunkCount++;
    if (lastChunk != null) {
        lastChunk->nextChunk = newChunk;
    }
    lastChunk = newChunk;
    if (firstChunk == null) {
        firstChunk = newChunk;
    }
}

template<typename T>
ArrayChunk<T>* ChunkedArray<T>::getChunkByIndex(s32 chunkIndex)
{
    ASSERT(chunkIndex >= 0 && chunkIndex < chunkCount); // chunkIndex is out of range!

    ArrayChunk<T>* chunk = null;

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

template<typename T>
ArrayChunk<T>* ChunkedArray<T>::getLastNonEmptyChunk()
{
    ArrayChunk<T>* lastNonEmptyChunk = lastChunk;

    if (count > 0) {
        while (lastNonEmptyChunk->count == 0) {
            lastNonEmptyChunk = lastNonEmptyChunk->prevChunk;
        }
    }

    return lastNonEmptyChunk;
}

template<typename T>
void ChunkedArray<T>::returnLastChunkToPool()
{
    ArrayChunk<T>* chunk = lastChunk;

    ASSERT(chunk->count == 0); // Attempting to return a non-empty chunk to the chunk pool!
    lastChunk = lastChunk->prevChunk;
    if (firstChunk == chunk)
        firstChunk = lastChunk;
    chunkCount--;

    addItemToPool<ArrayChunk<T>>(chunkPool, chunk);
}

template<typename T>
template<typename Comparison>
void ChunkedArray<T>::sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex)
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

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

template<typename T>
inline bool ChunkedArrayIterator<T>::hasNext()
{
    return !isDone;
}

template<typename T>
void ChunkedArrayIterator<T>::next()
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

            if (currentChunk == null) {
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

            if (currentChunk == null) {
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

template<typename T>
inline T* ChunkedArrayIterator<T>::get()
{
    return &currentChunk->items[indexInChunk];
}

template<typename T>
s32 ChunkedArrayIterator<T>::getIndex()
{
    return (chunkIndex * array->itemsPerChunk) + indexInChunk;
}

template<typename T>
inline T ChunkedArrayIterator<T>::getValue()
{
    return currentChunk->items[indexInChunk];
}
