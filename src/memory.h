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
	bool bootstrapSucceeded = initMemoryArena(&bootstrap, sizeof(containerType)); \
	ASSERT(bootstrapSucceeded); \
	containerName = allocateStruct<containerType>(&bootstrap);                            \
	containerName->arenaVarName = bootstrap;                                          \
	markResetPosition(&containerName->arenaVarName);                                  \
}

//
// Creates a struct T whose member at offsetOfArenaMember is a MemoryArena which will contain
// the memory for T. So, it kind of contains itself.
// Usage:
// Blah *blah = bootstrapMemoryArena<Blah>(offsetof(Blah, nameOfArenaInBlah));
// Because of the limitations of templates, there's no way to do this in a checked way...
// if you pass the wrong offset, it has no way of knowing and will still run, but in a subtly-buggy,
// memory-corrupting way. So, I've decided it's better to stick with the macro version above!
// Keeping this here for posterity though. Maybe one day I'll figure out a better way of doing it.
// My template-fu is still quite lacking.
//
// - Sam, 29/06/2019
//
/*
template<typename T>
T *bootstrapMemoryArena(smm offsetOfArenaMember)
{
	T *result = null;

	MemoryArena bootstrap;
	initMemoryArena(&bootstrap, sizeof(T));
	result = allocateStruct<T>(&bootstrap);

	MemoryArena *arena = (MemoryArena *)((u8*)result + offsetOfArenaMember);
	*arena = bootstrap;
	markResetPosition(arena);

	return result;
}
*/

bool initMemoryArena(MemoryArena *arena, smm size, smm minimumBlockSize=MB(1));
void markResetPosition(MemoryArena *arena);
void resetMemoryArena(MemoryArena *arena);
void revertMemoryArena(MemoryArena *arena, MemoryArenaResetState resetState);
void freeMemoryArena(MemoryArena *arena);

u8 *allocateRaw(smm size);
void *allocate(MemoryArena *arena, smm size);
Blob allocateBlob(MemoryArena *arena, smm size);

template<typename T>
T *allocateStruct(MemoryArena *arena);

template<typename T>
T *allocateArray(MemoryArena *arena, smm count);

template<typename T>
void copyMemory(T *source, T *dest, smm length);

template<typename T>
bool isMemoryEqual(T *a, T *b, smm length=1);
