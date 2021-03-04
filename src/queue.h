#pragma once

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
struct QueueIterator;

template <typename T>
struct Queue
{
	bool isEmpty();
	T *push();
	T *push(T item);
	Maybe<T*> peek();
	Maybe<T> pop();

	QueueIterator<T> iterate(bool goBackwards = false);

	// "private"
	Pool<QueueChunk<T>> chunkPool;
	s32 chunkSize;

	s32 count;

	QueueChunk<T> *startChunk;
	QueueChunk<T> *endChunk;
};

template <typename T>
void initQueue(Queue<T> *queue, MemoryArena *arena, s32 chunkSize = 32);

template <typename T>
struct QueueIterator
{
	Queue<T> *queue;
	bool goBackwards;

	QueueChunk<T> *currentChunk;
	s32 indexInChunk;

	bool isDone;


	bool hasNext();
	void next();
	T *get();
};
