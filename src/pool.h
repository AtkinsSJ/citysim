#pragma once

struct PoolItem
{
	PoolItem *prevPoolItem;
	PoolItem *nextPoolItem;
};

template<typename T>
struct Pool
{
	MemoryArena *memoryArena;
	T *(*allocateItem)(MemoryArena *arena);

	smm count;
	T *firstItem;
};

template<typename T>
void initPool(Pool<T> *pool, MemoryArena *arena, T *(*allocateItem)(MemoryArena *arena) = allocateStruct<T>)
{
	pool->memoryArena = arena;
	pool->allocateItem = allocateItem;
	pool->count = 0;
	pool->firstItem = null;
}

template<typename T>
T *getItemFromPool(Pool<T> *pool)
{
	T *result = null;

	if (pool->count > 0)
	{
		result = pool->firstItem;
		pool->firstItem = (T*)result->nextPoolItem;
		pool->count--;
		if (pool->firstItem != null)
		{
			pool->firstItem->prevPoolItem = null;
		}
	}
	else
	{
		result = pool->allocateItem(pool->memoryArena);
	}

	result->prevPoolItem = null;
	result->nextPoolItem = null;

	return result;
}

template<typename T>
void addItemToPool(T *item, Pool<T> *pool)
{
	// Remove item from its existing list
	if (item->prevPoolItem != null) item->prevPoolItem->nextPoolItem = item->nextPoolItem;
	if (item->nextPoolItem != null) item->nextPoolItem->prevPoolItem = item->prevPoolItem;

	// Add to the pool
	item->prevPoolItem = null;
	item->nextPoolItem = pool->firstItem;
	if (pool->firstItem != null)
	{
		pool->firstItem->prevPoolItem = item;
	}
	pool->firstItem = item;
	pool->count++;
}
