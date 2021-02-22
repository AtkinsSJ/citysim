#pragma once

template <typename T>
struct Queue
{
	Queue(MemoryArena *arena, s32 initialSize)
		: arena(arena),
		  chunkSize(initialSize)
		{}

	bool isEmpty();
	T *push(T item);
	Maybe<T> pop();

	// "private"
	MemoryArena *arena;
	ChunkPool<T> chunkPool;
	s32 chunkSize;

	s32 count = 0;

	Chunk<T> *startChunk;

};
