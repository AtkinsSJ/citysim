#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h> // For calloc
#include <string.h> // For memset

#define KB(x) ((x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

// NB: MemoryBlock is positioned just before its memory pointer.
// So when deallocating, we can just free(block)!
struct MemoryBlock
{
	MemoryBlock *prevBlock;

	umm size;
	umm used;
	u8 *memory;
};

struct MemoryArenaResetState
{
	MemoryBlock *currentBlock;
	umm used;
};

struct MemoryArena
{
	MemoryBlock *currentBlock;

	umm minimumBlockSize;
	bool hasTemporaryArenaOpen;

	MemoryArenaResetState resetState;
};

struct TemporaryMemory
{
	MemoryArena *arena;
	bool isOpen;

	MemoryArenaResetState resetState;
};

MemoryBlock *addMemoryBlock(MemoryArena *arena, umm size)
{
	umm totalSize = size + sizeof(MemoryBlock);
	u8* memory = (u8*) calloc(totalSize, 1);

	ASSERT(memory, "Failed to allocate memory block!");

	MemoryBlock *block = (MemoryBlock*) memory;
	block->memory = memory + sizeof(MemoryBlock);
	block->used = 0;
	block->size = size;

	block->prevBlock = arena->currentBlock;

	return block;
}

void markResetPosition(MemoryArena *arena)
{
	arena->resetState.currentBlock = arena->currentBlock;
	arena->resetState.used = arena->currentBlock ? arena->currentBlock->used : 0;
}

bool initMemoryArena(MemoryArena *arena, umm size, umm minimumBlockSize=MB(1))
{
	bool succeeded = true;

	*arena = {};
	arena->minimumBlockSize = minimumBlockSize;

	if (size)
	{
		arena->currentBlock = addMemoryBlock(arena, size);
		succeeded = (arena->currentBlock->memory != 0);
	}

	markResetPosition(arena);
	
	return succeeded;
}

// Creates an arena , and pushes a struct on it which contains the arena.
#define bootstrapArena(containerType, containerName, arenaVarName)         \
{                                                                                     \
	MemoryArena bootstrap;                                                            \
	ASSERT(initMemoryArena(&bootstrap, sizeof(containerType)),"Failed to allocate memory for %s arena!", #containerType);\
	containerName = PushStruct(&bootstrap, containerType);                            \
	containerName->arenaVarName = bootstrap;                                          \
	markResetPosition(&containerName->arenaVarName);                                  \
}

void *allocate(MemoryArena *arena, umm size)
{
	if ((arena->currentBlock == 0)
		|| (arena->currentBlock->used + size > arena->currentBlock->size))
	{
		umm newBlockSize = MAX(size, arena->minimumBlockSize);
		arena->currentBlock = addMemoryBlock(arena, newBlockSize);
	}

	ASSERT(arena->currentBlock, "No memory in arena!");

	// TODO: Prevent normal allocations while temp mem is open, and vice versa
	// We tried passing an isTempAllocation bool, but code that just takes a MemoryArena doesn't know
	// what kind of memory it's allocating from so it fails.
	// For it to work we'd have to duplicate every function that takes an arena eg readFile()
	// ASSERT(isTempAllocation == arena->hasTemporaryArenaOpen, "Mixing temporary and regular allocations!");
	
	void *result = arena->currentBlock->memory + arena->currentBlock->used;
	memset(result, 0, size);
	
	arena->currentBlock->used += size;
	
	return result;
}

void *allocate(TemporaryMemory *tempArena, umm size)
{
	ASSERT(tempArena->isOpen, "TemporaryMemory already ended!");
	return allocate(tempArena->arena, size);
}

void freeCurrentBlock(MemoryArena *arena)
{
	MemoryBlock *block = arena->currentBlock;
	ASSERT(block, "Attempting to free non-existent block");
	arena->currentBlock = block->prevBlock;
	free(block);
}

// Returns the memory arena to a previous state
void revertMemoryArena(MemoryArena *arena, MemoryArenaResetState resetState)
{
	ASSERT(!arena->hasTemporaryArenaOpen, "Can't reset while temp memory open!");

	while (arena->currentBlock != resetState.currentBlock)
	{
		freeCurrentBlock(arena);
	}

	if (arena->currentBlock)
	{
#if BUILD_DEBUG
		// Clear memory so we spot bugs in keeping pointers to deallocated memory.
		memset(arena->currentBlock->memory + resetState.used, 0,
			   arena->currentBlock->used - resetState.used);
#endif
		arena->currentBlock->used = resetState.used;
	}
}

void resetMemoryArena(MemoryArena *arena)
{
	revertMemoryArena(arena, arena->resetState);
}

void freeMemoryArena(MemoryArena *arena)
{
	// Free all but the original block
	while (arena->currentBlock->prevBlock)
	{
		freeCurrentBlock(arena);
	}

	// Free original block, which may contain the arena so we have to be careful!
	MemoryBlock *finalBlock = arena->currentBlock;
	arena->currentBlock = 0;
	free(finalBlock);
}

TemporaryMemory beginTemporaryMemory(MemoryArena *parentArena)
{
	ASSERT(!parentArena->hasTemporaryArenaOpen, "Beginning temporary memory without ending it!");
	if (parentArena->currentBlock == 0)
	{
		logWarn("Starting temporary memory on an empty arena! Wasteful if this is in frequently used code!");
	}

	TemporaryMemory tempMemory = {};

	tempMemory.isOpen = true;
	tempMemory.arena = parentArena;
	parentArena->hasTemporaryArenaOpen = true;
	
	tempMemory.resetState.currentBlock = parentArena->currentBlock;
	tempMemory.resetState.used = parentArena->currentBlock ? parentArena->currentBlock->used : 0;

	return tempMemory;
}

void endTemporaryMemory(TemporaryMemory *tempMemory)
{
	MemoryArena *arena = tempMemory->arena;
	ASSERT(arena->hasTemporaryArenaOpen, "Ending temporary memory without beginning it!");
	arena->hasTemporaryArenaOpen = false;

	revertMemoryArena(arena, tempMemory->resetState);

	tempMemory->isOpen = false;
	tempMemory->arena = 0; //null so we get a null pointer if we try to access it after ending.
}

#define PushStruct(Arena, Struct) ((Struct*)allocate(Arena, sizeof(Struct)))
#define PushArray(Arena, Type, Count) ((Type*)allocate(Arena, sizeof(Type) * Count))

#endif
