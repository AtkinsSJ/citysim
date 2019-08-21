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
struct OccupancyArrayItem
{
	s32 index;
	T *item;
};
template<typename T>
OccupancyArrayItem<T> append(OccupancyArray<T> *array);

template<typename T>
T removeIndex(OccupancyArray<T> *array, s32 indexToRemove);

template<typename T>
T *get(OccupancyArray<T> *array, s32 index);
