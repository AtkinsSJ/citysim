/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MemoryArena.h"
#include "Log.h"
#include "Maths.h"
#include "Memory.h"

inline Blob allocateBlob(MemoryArena* arena, smm size)
{
    return makeBlob(size, (u8*)allocate(arena, size));
}

MemoryBlock* addMemoryBlock(MemoryArena* arena, smm size)
{
    smm totalSize = size + sizeof(MemoryBlock);
    u8* memory = allocateRaw(totalSize);

    ASSERT(memory != nullptr); // Failed to allocate memory block!

    MemoryBlock* block = (MemoryBlock*)memory;
    block->memory = memory + sizeof(MemoryBlock);
    block->used = 0;
    block->size = size;

    block->prevBlock = arena->currentBlock;

    return block;
}

void* allocate(MemoryArena* arena, smm size)
{
    if (size > arena->minimumBlockSize) {
        logWarn("Large allocation in {0} arena: {1} bytes when block size is {2} bytes"_s, { arena->name, formatInt(size), formatInt(arena->minimumBlockSize) });
    }

    ASSERT(size < GB(1)); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    if ((arena->currentBlock == 0)
        || (arena->currentBlock->used + size > arena->currentBlock->size)) {
        smm newBlockSize = max(size, arena->minimumBlockSize);
        arena->currentBlock = addMemoryBlock(arena, newBlockSize);
    }

    ASSERT(arena->currentBlock != nullptr); // No memory in arena!

    void* result = arena->currentBlock->memory + arena->currentBlock->used;
    memset(result, 0, size);

    arena->currentBlock->used += size;

    return result;
}

void freeCurrentBlock(MemoryArena* arena)
{
    MemoryBlock* block = arena->currentBlock;
    ASSERT(block != nullptr); // Attempting to free non-existent block
    arena->currentBlock = block->prevBlock;
    deallocateRaw(block);
}

// Returns the memory arena to a previous state
void revertMemoryArena(MemoryArena* arena, MemoryArenaResetState resetState)
{
    while (arena->currentBlock != resetState.currentBlock) {
        freeCurrentBlock(arena);
    }

    if (arena->currentBlock) {
#if BUILD_DEBUG
        // Clear memory so we spot bugs in keeping pointers to deallocated memory.
        fillMemory<u8>(arena->currentBlock->memory + resetState.used, 0xcd, arena->currentBlock->used - resetState.used);
#endif
        arena->currentBlock->used = resetState.used;
    }
}

void resetMemoryArena(MemoryArena* arena)
{
    revertMemoryArena(arena, arena->resetState);
}

void freeMemoryArena(MemoryArena* arena)
{
    // Free all but the original block
    while (arena->currentBlock->prevBlock) {
        freeCurrentBlock(arena);
    }

    // Free original block, which may contain the arena so we have to be careful!
    MemoryBlock* finalBlock = arena->currentBlock;
    arena->currentBlock = 0;
    deallocateRaw(finalBlock);
}

MemoryArenaResetState getArenaPosition(MemoryArena* arena)
{
    MemoryArenaResetState result = {};

    result.currentBlock = arena->currentBlock;
    result.used = arena->currentBlock ? arena->currentBlock->used : 0;

    return result;
}

void markResetPosition(MemoryArena* arena)
{
    arena->resetState = getArenaPosition(arena);
}

bool initMemoryArena(MemoryArena* arena, String name, smm size, smm minimumBlockSize)
{
    bool succeeded = true;

    *arena = {};
    arena->minimumBlockSize = minimumBlockSize;

    if (size) {
        arena->currentBlock = addMemoryBlock(arena, max(size, minimumBlockSize));
        succeeded = (arena->currentBlock->memory != 0);
    }

    arena->name = pushString(arena, name);

    markResetPosition(arena);

    return succeeded;
}
