#pragma once

#include <stdlib.h> // For calloc
#include <string.h> // For memset

#define KB(x) ((x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

struct Blob
{
	smm size;
	u8 *memory;
};

// NB: MemoryBlock is positioned just before its memory pointer.
// So when deallocating, we can just free(block)!
struct MemoryBlock
{
	MemoryBlock *prevBlock;

	smm size;
	smm used;
	u8 *memory;
};

struct MemoryArenaResetState
{
	MemoryBlock *currentBlock;
	smm used;
};

struct MemoryArena
{
	MemoryBlock *currentBlock;

	smm minimumBlockSize;

	MemoryArenaResetState resetState;
};

// Creates an arena, and pushes a struct on it which contains the arena.
#define bootstrapArena(containerType, containerName, arenaVarName)  \
{                                                                                     \
	MemoryArena bootstrap;                                                            \
	ASSERT(initMemoryArena(&bootstrap, sizeof(containerType)),"Failed to allocate memory for {0} arena!", {makeString(#containerType)});\
	containerName = allocateStruct<containerType>(&bootstrap);                            \
	containerName->arenaVarName = bootstrap;                                          \
	markResetPosition(&containerName->arenaVarName);                                  \
}

bool initMemoryArena(MemoryArena *arena, smm size, smm minimumBlockSize=MB(1));
void markResetPosition(MemoryArena *arena);

u8 *allocateRaw(smm size);
void *allocate(MemoryArena *arena, smm size);
Blob allocateBlob(MemoryArena *arena, smm size);

template<typename T>
T *allocateStruct(MemoryArena *arena);

template<typename T>
T *allocateArray(MemoryArena *arena, smm count);

template<typename T>
void copyMemory(T *source, T *dest, smm length);
