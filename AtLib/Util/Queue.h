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
class Queue {
public:
    // FIXME: Temporary until initialization is more sane everywhere.
    Queue() = default;

    explicit Queue(MemoryArena& arena, s32 chunk_size = 32)
        : m_chunk_size(chunk_size)
    {
        initPool<QueueChunk<T>>(&m_chunk_pool, &arena, [](MemoryArena* arena, void* userData) {
                    s32 chunk_size = *static_cast<s32*>(userData);

                    auto [new_chunk, items] = arena->allocate_with_data<QueueChunk<T>, T>(chunk_size);
                    new_chunk.items = items.raw_data();
                    return &new_chunk; }, &m_chunk_size);

        // We're starting off with one chunk, even though it's empty. Perhaps we should wait
        // until we actually add something? I'm not sure.
        m_start_chunk = getItemFromPool(&m_chunk_pool);
        m_end_chunk = m_start_chunk;
    }

    bool is_empty() const { return m_count == 0; }

    template<typename... Args>
    T& push(Args&&... args)
    {
        if (m_end_chunk == nullptr) {
            // In case we don't yet have a chunk, or we became empty and removed it, add one.
            ASSERT(m_start_chunk == nullptr); // If we have a start but no end, something has gone very wrong!!!
            m_start_chunk = getItemFromPool(&m_chunk_pool);
            m_start_chunk->count = 0;
            m_start_chunk->startIndex = 0;
            m_end_chunk = m_start_chunk;
        } else if (m_end_chunk->count == m_chunk_size) {
            // We're full, so get a new chunk
            QueueChunk<T>* newChunk = getItemFromPool(&m_chunk_pool);
            newChunk->count = 0;
            newChunk->startIndex = 0;
            newChunk->prevChunk = m_end_chunk;
            m_end_chunk->nextChunk = newChunk;

            m_end_chunk = newChunk;
        }

        void* data = m_end_chunk->items + m_end_chunk->startIndex + m_end_chunk->count;
        m_end_chunk->count++;
        m_count++;

        return *new (data) T(forward<Args>(args)...);
    }

    Optional<T&> peek() const
    {
        if (!is_empty())
            return m_start_chunk->items[m_start_chunk->startIndex];

        return {};
    }

    Optional<T> pop()
    {
        if (!is_empty()) {
            Optional<T> result = move(m_start_chunk->items[m_start_chunk->startIndex]);

            m_start_chunk->count--;
            m_start_chunk->startIndex++;
            m_count--;

            if (m_start_chunk->count == 0) {
                // We're empty! So return it to the pool.
                // EXCEPT: If this is the only chunk, we might as well keep it and just clear it!
                if (m_start_chunk == m_end_chunk) {
                    m_start_chunk->startIndex = 0;
                } else {
                    QueueChunk<T>* newStartChunk = m_start_chunk->nextChunk;
                    if (newStartChunk != nullptr) {
                        newStartChunk->prevChunk = nullptr;
                    }

                    addItemToPool(&m_chunk_pool, m_start_chunk);
                    m_start_chunk = newStartChunk;

                    // If we just removed the only chunk, make sure to clear the endChunk to match.
                    if (m_start_chunk == nullptr) {
                        m_end_chunk = nullptr;
                    }
                }
            }
            return result;
        }

        return {};
    }

    QueueIterator<T> iterate(bool goBackwards = false)
    {
        QueueIterator<T> result = {};

        result.queue = this;
        result.goBackwards = goBackwards;
        result.isDone = is_empty();

        if (!result.isDone) {
            if (goBackwards) {
                result.currentChunk = m_end_chunk;
                result.indexInChunk = m_end_chunk->startIndex + m_end_chunk->count - 1;
            } else {
                result.currentChunk = m_start_chunk;
                result.indexInChunk = m_start_chunk->startIndex;
            }
        }

        return result;
    }

private:
    DeprecatedPool<QueueChunk<T>> m_chunk_pool;
    s32 m_chunk_size;

    s32 m_count { 0 };

    QueueChunk<T>* m_start_chunk;
    QueueChunk<T>* m_end_chunk;
};

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
