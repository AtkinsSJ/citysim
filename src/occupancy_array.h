#pragma once

template<typename T>
struct OccupancyArrayChunk
{
	s32 count; // @Size: This is redundant as the BitArray tracks the number of set bits too.
	T *items;
	BitArray occupancy;

	OccupancyArrayChunk<T> *prevChunk;
	OccupancyArrayChunk<T> *nextChunk;
};

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
};

template<typename T>
OccupancyArrayIterator<T> iterate(OccupancyArray<T> *array);

template<typename T>
bool hasNext(OccupancyArrayIterator<T> *iterator);

template<typename T>
void next(OccupancyArrayIterator<T> *iterator);

template<typename T>
T *get(OccupancyArrayIterator<T> *iterator);

template<typename T>
s32 getIndex(OccupancyArrayIterator<T> *iterator);
