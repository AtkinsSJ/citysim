#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h> // For calloc
#include <string.h>

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024 * 1024 * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

struct MemoryBlock
{
	MemoryBlock *prevBlock;
	umm size;
	umm used;
	umm usedResetPosition; // When used goes when reset. If >0 block can't be deallocated
	uint8 *memory;
};

struct MemoryArena
{
	//MemoryBlock *currentBlock;

	umm size;
	umm used;
	umm usedResetPosition; // When used goes when reset
	bool hasTemporaryArenaOpen;
	uint8 *memory;
};

struct TemporaryMemory
{
	MemoryArena *arena;
	umm oldUsed;
	bool isOpen;
};

bool initMemoryArena(MemoryArena *arena, umm size)
{
	bool succeeded = false;

	*arena = {};

	arena->memory = (uint8*)calloc(size, 1);
	if (arena->memory)
	{
		arena->size = size;
		succeeded = true;
	}
	
	return succeeded;
}

void markResetPosition(MemoryArena *arena)
{
	arena->usedResetPosition = arena->used;
}

// Creates an arena , and pushes a struct on it which contains the arena.
#define bootstrapArena(containerType, containerName, arenaVarName, arenaSize)         \
{                                                                                     \
	MemoryArena bootstrap;                                                            \
	ASSERT(initMemoryArena(&bootstrap, arenaSize),"Failed to allocate memory for %s arena!", #containerType);\
	containerName = PushStruct(&bootstrap, containerType);                            \
	containerName->arenaVarName = bootstrap;                                          \
	markResetPosition(&containerName->arenaVarName);                                  \
}

void *allocate(MemoryArena *arena, umm size)
{
	ASSERT((arena->used + size) <= arena->size, "Arena out of memory!");
	// TODO: Prevent normal allocations while temp mem is open, and vice versa
	// We tried passing an isTempAllocation bool, but code that just takes a MemoryArena doesn't know
	// what kind of memory it's allocating from so it fails.
	// For it to work we'd have to duplicate every function that takes an arena eg readFile()
	// ASSERT(isTempAllocation == arena->hasTemporaryArenaOpen, "Mixing temporary and regular allocations!");
	
	void *result = arena->memory + arena->used;
	memset(result, 0, size);
	
	arena->used += size;
	
	return result;
}

void *allocate(TemporaryMemory *tempArena, umm size)
{
	ASSERT(tempArena->isOpen, "TemporaryMemory already ended!");
	return allocate(tempArena->arena, size);
}

void resetMemoryArena(MemoryArena *arena)
{
	ASSERT(!arena->hasTemporaryArenaOpen, "Can't reset while temp memory open!");
	arena->used = arena->usedResetPosition;
}

TemporaryMemory beginTemporaryMemory(MemoryArena *parentArena)
{
	ASSERT(!parentArena->hasTemporaryArenaOpen, "Beginning temporary memory without ending it!");

	TemporaryMemory tempMemory = {};

	tempMemory.isOpen = true;
	tempMemory.arena = parentArena;
	parentArena->hasTemporaryArenaOpen = true;
	tempMemory.oldUsed = parentArena->used;

	return tempMemory;
}

void endTemporaryMemory(TemporaryMemory *tempArena)
{
#if BUILD_DEBUG
	// Clear memory so we spot bugs in keeping pointers to temp memory.
	memset(tempArena->arena->memory + tempArena->oldUsed, 0,
		   tempArena->arena->used - tempArena->oldUsed);
#endif
	tempArena->isOpen = false;
	tempArena->arena->hasTemporaryArenaOpen = false;
	tempArena->arena->used = tempArena->oldUsed;
	tempArena->arena = 0; //null so we get a null pointer if we try to access it after ending.
}

#define PushStruct(Arena, Struct) ((Struct*)allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)allocate(Arena, sizeof(Type) * Count))

char *pushString(MemoryArena *arena, char *src)
{
	int32 len = strlen(src);
	char *dest = PushArray(arena, char, len+1);
	strcpy(dest, src);
	dest[len] = 0;
	return dest;
}

#endif
