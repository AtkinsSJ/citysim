/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/DeprecatedPool.h>
#include <Util/Forward.h>
#include <Util/Indexed.h>
#include <Util/MemoryArena.h>

//
// Queue! Your standard FIFO data structure
//
// Internally this works using a linked list of arrays of items. When the end
// chunk fills up, we add a new chunk. When the first chunk becomes empty, we
// remove it and point to the next chunk. Chunks are pooled so that we're not
// making unnecessary allocations - a queue doesn't make any allocations
// except to become larger.
//
// Queues don't share pools at this time, (03/03/2021) but that would not be a
// difficult change to make.
//

template<typename T>
struct QueueChunk : PoolItem {
    s32 count;
    s32 startIndex;
    T* items;

    QueueChunk<T>* prevChunk;
    QueueChunk<T>* nextChunk;
};

template<typename T>
struct QueueIterator;

template<typename T>
struct Queue {
    bool isEmpty() { return count == 0; }

    T* push()
    {
        if (endChunk == nullptr) {
            // In case we don't yet have a chunk, or we became empty and removed it, add one.
            ASSERT(startChunk == nullptr); // If we have a start but no end, something has gone very wrong!!!
            startChunk = getItemFromPool(&chunkPool);
            startChunk->count = 0;
            startChunk->startIndex = 0;
            endChunk = startChunk;
        } else if (endChunk->count == chunkSize) {
            // We're full, so get a new chunk
            QueueChunk<T>* newChunk = getItemFromPool(&chunkPool);
            newChunk->count = 0;
            newChunk->startIndex = 0;
            newChunk->prevChunk = endChunk;
            endChunk->nextChunk = newChunk;

            endChunk = newChunk;
        }

        T* result = endChunk->items + endChunk->startIndex + endChunk->count;
        endChunk->count++;
        count++;

        return result;
    }

    T* push(T item)
    {
        T* result = push();
        *result = item;
        return result;
    }

    Maybe<T*> peek()
    {
        Maybe<T*> result = makeFailure<T*>();

        if (!isEmpty()) {
            result = makeSuccess(startChunk->items + startChunk->startIndex);
        }

        return result;
    }

    Maybe<T> pop()
    {
        Maybe<T> result = makeFailure<T>();

        if (!isEmpty()) {
            result = makeSuccess(startChunk->items[startChunk->startIndex]);

            startChunk->count--;
            startChunk->startIndex++;
            count--;

            if (startChunk->count == 0) {
                // We're empty! So return it to the pool.
                // EXCEPT: If this is the only chunk, we might as well keep it and just clear it!
                if (startChunk == endChunk) {
                    startChunk->startIndex = 0;
                } else {
                    QueueChunk<T>* newStartChunk = startChunk->nextChunk;
                    if (newStartChunk != nullptr) {
                        newStartChunk->prevChunk = nullptr;
                    }

                    addItemToPool(&chunkPool, startChunk);
                    startChunk = newStartChunk;

                    // If we just removed the only chunk, make sure to clear the endChunk to match.
                    if (startChunk == nullptr) {
                        endChunk = nullptr;
                    }
                }
            }
        }

        return result;
    }

    QueueIterator<T> iterate(bool goBackwards = false)
    {
        QueueIterator<T> result = {};

        result.queue = this;
        result.goBackwards = goBackwards;
        result.isDone = isEmpty();

        if (!result.isDone) {
            if (goBackwards) {
                result.currentChunk = this->endChunk;
                result.indexInChunk = this->endChunk->startIndex + this->endChunk->count - 1;
            } else {
                result.currentChunk = this->startChunk;
                result.indexInChunk = this->startChunk->startIndex;
            }
        }

        return result;
    }

    // "private"
    DeprecatedPool<QueueChunk<T>> chunkPool;
    s32 chunkSize;

    s32 count;

    QueueChunk<T>* startChunk;
    QueueChunk<T>* endChunk;
};

template<typename T>
void initQueue(Queue<T>* queue, MemoryArena* arena, s32 chunkSize = 32)
{
    *queue = {};

    queue->chunkSize = chunkSize;

    initPool<QueueChunk<T>>(&queue->chunkPool, arena, [](MemoryArena* arena, void* userData) {
                s32 chunkSize = *(s32*)userData;

                u8 *bytes = (u8*) arena->allocate_deprecated(sizeof(QueueChunk<T>) + (sizeof(T) * chunkSize));
                QueueChunk<T> *newChunk = (QueueChunk<T> *)bytes;
                *newChunk = {};
                newChunk->items = (T *)(bytes + sizeof(QueueChunk<T>));

                return newChunk; }, &queue->chunkSize);

    // We're starting off with one chunk, even though it's empty. Perhaps we should wait
    // until we actually add something? I'm not sure.
    queue->startChunk = getItemFromPool(&queue->chunkPool);
    queue->endChunk = queue->startChunk;
}

template<typename T>
struct QueueIterator {
    Queue<T>* queue;
    bool goBackwards;

    QueueChunk<T>* currentChunk;
    s32 indexInChunk;

    bool isDone;

    bool hasNext() { return !isDone; }

    void next()
    {
        if (isDone)
            return;

        if (goBackwards) {
            indexInChunk--;
            if (indexInChunk < currentChunk->startIndex) {
                // Previous chunk
                if (currentChunk->prevChunk == nullptr) {
                    // We're done!
                    isDone = true;
                } else {
                    currentChunk = currentChunk->prevChunk;
                    indexInChunk = currentChunk->startIndex + currentChunk->count - 1;
                }
            }
        } else {
            indexInChunk++;
            if (indexInChunk >= currentChunk->count + currentChunk->startIndex) {
                // Next chunk
                if (currentChunk->nextChunk == nullptr) {
                    // We're done!
                    isDone = true;
                } else {
                    currentChunk = currentChunk->nextChunk;
                    indexInChunk = currentChunk->startIndex;
                }
            }
        }
    }

    T* get() { return currentChunk->items + indexInChunk; }
};
