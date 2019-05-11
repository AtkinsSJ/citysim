#ifndef MEMORY_H
#define MEMORY_H

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

// Creates an arena , and pushes a struct on it which contains the arena.
#define bootstrapArena(containerType, containerName, arenaVarName)         \
{                                                                                     \
	MemoryArena bootstrap;                                                            \
	ASSERT(initMemoryArena(&bootstrap, sizeof(containerType)),"Failed to allocate memory for {0} arena!", {makeString(#containerType)});\
	containerName = PushStruct(&bootstrap, containerType);                            \
	containerName->arenaVarName = bootstrap;                                          \
	markResetPosition(&containerName->arenaVarName);                                  \
}

#define PushStruct(Arena, Struct) ((Struct*)allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)allocate(Arena, sizeof(Type) * Count))

bool initMemoryArena(MemoryArena *arena, smm size, smm minimumBlockSize=MB(1));
void markResetPosition(MemoryArena *arena);
void *allocate(MemoryArena *arena, smm size);
u8 *allocateRaw(smm size)
{
	u8 *result = (u8*) calloc(size, 1);
	ASSERT(result != null, "calloc() failed!!! I don't think there's anything reasonable we can do here.");
	return result;
}

#endif
