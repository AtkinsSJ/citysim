#pragma once

template<typename T>
struct OccupancyArrayChunk
{
	smm count;
	smm maxCount;
	T *items;
	BitArray occupancy;

	OccupancyArrayChunk<T> *prevChunk;
	OccupancyArrayChunk<T> *nextChunk;
};

template<typename T>
struct OccupancyArray
{
	MemoryArena *memoryArena;

	smm itemsPerChunk;
	smm chunkCount;
	smm count;

	OccupancyArrayChunk<T> *firstChunk;
	OccupancyArrayChunk<T> *lastChunk;

	smm firstChunkWithSpaceIndex;
	OccupancyArrayChunk<T> *firstChunkWithSpace;
};

template<typename T>
void initOccupancyArray(OccupancyArray<T> *array, MemoryArena *arena, smm itemsPerChunk);

template<typename T>
struct OccupancyArrayItem
{
	smm index;
	T *item;
};
template<typename T>
OccupancyArrayItem<T> append(OccupancyArray<T> *array);

template<typename T>
T removeIndex(OccupancyArray<T> *array, smm indexToRemove);

template<typename T>
T *get(OccupancyArray<T> *array, smm index);
