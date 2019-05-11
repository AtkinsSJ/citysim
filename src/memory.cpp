#pragma once

Blob allocateBlob(MemoryArena *arena, smm size)
{
	Blob result = {};

	result.size = size;
	result.memory = (u8*) allocate(arena, size);

	return result;
}

MemoryBlock *addMemoryBlock(MemoryArena *arena, smm size)
{
	smm totalSize = size + sizeof(MemoryBlock);
	u8* memory = allocateRaw(totalSize);

	ASSERT(memory != null, "Failed to allocate memory block!");

	MemoryBlock *block = (MemoryBlock*) memory;
	block->memory = memory + sizeof(MemoryBlock);
	block->used = 0;
	block->size = size;

	block->prevBlock = arena->currentBlock;

	return block;
}

void *allocate(MemoryArena *arena, smm size)
{
	if ((arena->currentBlock == 0)
		|| (arena->currentBlock->used + size > arena->currentBlock->size))
	{
		smm newBlockSize = MAX(size, arena->minimumBlockSize);
		arena->currentBlock = addMemoryBlock(arena, newBlockSize);
	}

	ASSERT(arena->currentBlock != null, "No memory in arena!");

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

void freeCurrentBlock(MemoryArena *arena)
{
	MemoryBlock *block = arena->currentBlock;
	ASSERT(block != null, "Attempting to free non-existent block");
	arena->currentBlock = block->prevBlock;
	free(block);
}

// Returns the memory arena to a previous state
void revertMemoryArena(MemoryArena *arena, MemoryArenaResetState resetState)
{
	while (arena->currentBlock != resetState.currentBlock)
	{
		freeCurrentBlock(arena);
	}

	if (arena->currentBlock)
	{
#if BUILD_DEBUG
		// Clear memory so we spot bugs in keeping pointers to deallocated memory.
		memset(arena->currentBlock->memory + resetState.used, 0xcd,
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

void markResetPosition(MemoryArena *arena)
{
	arena->resetState.currentBlock = arena->currentBlock;
	arena->resetState.used = arena->currentBlock ? arena->currentBlock->used : 0;
}

bool initMemoryArena(MemoryArena *arena, smm size, smm minimumBlockSize)
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