#pragma once

template <typename T>
struct QueueChunk : PoolItem
{
	s32 count;
	s32 startIndex;
	T *items;

	QueueChunk<T> *prevChunk;
	QueueChunk<T> *nextChunk;
};

template <typename T>
struct Queue
{
	bool isEmpty();
	T *push(T item);
	Maybe<T> pop();

	// "private"
	Pool<QueueChunk<T>> chunkPool;
	s32 chunkSize;

	s32 count;

	QueueChunk<T> *startChunk;
	QueueChunk<T> *endChunk;
};

template <typename T>
void initQueue(Queue<T> *queue, MemoryArena *arena, s32 chunkSize = 32);
