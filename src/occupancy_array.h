#pragma once

template<typename T>
struct OccupancyArrayChunk
{
	T *items;
	BitArray occupancy;

	OccupancyArrayChunk<T> *prevChunk;
	OccupancyArrayChunk<T> *nextChunk;
};

template<typename T>
struct OccupancyArrayIterator;

// A chunked array that keeps track of which slots are empty, and attempts to fill them.
// (The regular ChunkedArray just moves the elements to fill empty slots, so the order can change.)
template<typename T>
struct OccupancyArray
{
	MemoryArena *memoryArena;

	s32 itemsPerChunk;
	s32 chunkCount;
	s32 count;

	OccupancyArrayChunk<T> *firstChunk;
	OccupancyArrayChunk<T> *lastChunk;

	s32 firstChunkWithSpaceIndex;
	OccupancyArrayChunk<T> *firstChunkWithSpace;

// Methods
	OccupancyArrayIterator<T> iterate();
};

template<typename T>
void initOccupancyArray(OccupancyArray<T> *array, MemoryArena *arena, s32 itemsPerChunk);

template<typename T>
Indexed<T*> append(OccupancyArray<T> *array);

template<typename T>
void removeIndex(OccupancyArray<T> *array, s32 indexToRemove);

template<typename T>
T *get(OccupancyArray<T> *array, s32 index);

template<typename T>
struct OccupancyArrayIterator
{
	OccupancyArray<T> *array;

	OccupancyArrayChunk<T> *currentChunk;
	s32 chunkIndex;
	s32 indexInChunk;

	bool isDone;

// Methods
	bool hasNext();
	void next();
	T *get();
	s32 getIndex();
};
