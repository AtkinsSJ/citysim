#pragma once

/**
 * General-purpose object pool.
 *
 * Saves memory allocations/deallocations by keeping a list of items that are not currently in use,
 * that can be taken from it when needed, and then returned to it later. It's mostly useful for when
 * there are multiple users of the items, and the number each of them needs varies a lot.
 * If there's only one user, it might as well keep them all itself!
 *
 * Use:
 * - Make your pooled item extend PoolItem.
 * - Call initPool(pool, arena to allocate from, allocation function, optional user data pointer);
 *		Allocation function is called whenever an item is requested from the pool and the pool is empty.
 *		User data pointer is passed to that allocation function, for when you need additional parameters.
 * - Call getItemFromPool(pool) when you want an item
 * - Call addItemToPool(item, pool) when you are done with the item
 *
 * - Sam, 26/07/2019
 */

struct PoolItem {
    PoolItem* prevPoolItem;
    PoolItem* nextPoolItem;
};

template<typename T>
struct Pool {
    MemoryArena* memoryArena;
    T* (*allocateItem)(MemoryArena* arena, void* userData);
    void* userData; // Passed to allocateItem()

    smm pooledItemCount; // How many items reside in the pool
    smm totalItemCount;  // How many items this pool has created, total

    T* firstItem;
};

template<typename T>
void initPool(Pool<T>* pool, MemoryArena* arena, T* (*allocateItem)(MemoryArena* arena, void* userData), void* userData = null)
{
    *pool = {};
    pool->memoryArena = arena;
    pool->allocateItem = allocateItem;
    pool->userData = userData;
    pool->firstItem = null;
    pool->pooledItemCount = 0;
    pool->totalItemCount = 0;
}

template<typename T>
T* getItemFromPool(Pool<T>* pool)
{
    T* result = null;

    if (pool->firstItem != null) {
        result = pool->firstItem;
        pool->firstItem = (T*)result->nextPoolItem;
        if (pool->firstItem != null) {
            pool->firstItem->prevPoolItem = null;
        }

        pool->pooledItemCount--;
    } else {
        result = pool->allocateItem(pool->memoryArena, pool->userData);
        pool->totalItemCount++;
    }

    result->prevPoolItem = null;
    result->nextPoolItem = null;

    return result;
}

template<typename T>
void addItemToPool(Pool<T>* pool, T* item)
{
    // Remove item from its existing list
    if (item->prevPoolItem != null)
        item->prevPoolItem->nextPoolItem = item->nextPoolItem;
    if (item->nextPoolItem != null)
        item->nextPoolItem->prevPoolItem = item->prevPoolItem;

    // Add to the pool
    item->prevPoolItem = null;
    item->nextPoolItem = pool->firstItem;
    if (pool->firstItem != null) {
        pool->firstItem->prevPoolItem = item;
    }
    pool->firstItem = item;

    pool->pooledItemCount++;
}
