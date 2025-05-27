#pragma once

inline Blob allocateBlob(MemoryArena* arena, smm size)
{
    return makeBlob(size, (u8*)allocate(arena, size));
}

MemoryBlock* addMemoryBlock(MemoryArena* arena, smm size)
{
    smm totalSize = size + sizeof(MemoryBlock);
    u8* memory = allocateRaw(totalSize);

    ASSERT(memory != null); // Failed to allocate memory block!

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

    ASSERT(arena->currentBlock != null); // No memory in arena!

    void* result = arena->currentBlock->memory + arena->currentBlock->used;
    memset(result, 0, size);

    arena->currentBlock->used += size;

    return result;
}

template<typename T>
inline T* allocateStruct(MemoryArena* arena)
{
    return (T*)allocate(arena, sizeof(T));
}

template<typename T>
inline T* allocateMultiple(MemoryArena* arena, smm count)
{
    // Negative allocations are obviously incorrect, so we assert them.
    // However, count=0 sometimes is used, eg when interning an empty string that's used
    // as a hashtable key. To avoid needlessly complicating user code, we just return null
    // here when you try to allocate 0 things. The end result is the same - you're not going
    // to use the memory if you've got 0 things allocated anyway!
    // - Sam, 28/03/2020
    ASSERT(count >= 0);
    T* result = null;

    if (count > 0) {
        result = (T*)allocate(arena, sizeof(T) * count);
    }

    return result;
}

template<typename T>
inline Array<T> allocateArray(MemoryArena* arena, s32 count, bool markAsFull)
{
    Array<T> result;

    if (count == 0) {
        result = makeEmptyArray<T>();
    } else {
        ASSERT(count > 0);
        result = makeArray<T>(count, allocateMultiple<T>(arena, count), markAsFull ? count : 0);
    }

    return result;
}

template<typename T>
inline Array2<T> allocateArray2(MemoryArena* arena, s32 w, s32 h)
{
    ASSERT(w > 0 && h > 0);
    return makeArray2<T>(w, h, allocateMultiple<T>(arena, w * h));
}

u8* allocateRaw(smm size)
{
    ASSERT(size < GB(1)); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    u8* result = (u8*)calloc(size, 1);
    ASSERT(result != null); // calloc() failed!!! I don't think there's anything reasonable we can do here.
    return result;
}

void deallocateRaw(void* memory)
{
    free(memory);
}

void freeCurrentBlock(MemoryArena* arena)
{
    MemoryBlock* block = arena->currentBlock;
    ASSERT(block != null); // Attempting to free non-existent block
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

template<typename T>
inline void copyMemory(T const* source, T* dest, smm length)
{
    memcpy(dest, source, length * sizeof(T));
}

template<typename T>
inline void fillMemory(T* memory, T value, smm length)
{
    T* currentElement = memory;
    smm remainingBytes = length * sizeof(T);

    // NB: The loop below works for a fractional number of Ts, but remainingBytes
    // is always a whole number of them! It's kind of weird. Could possibly make
    // this faster by restricting it to whole Ts.

    while (remainingBytes > 0) {
        smm toCopy = min<smm>(remainingBytes, sizeof(T));
        memcpy(currentElement, &value, toCopy);
        remainingBytes -= toCopy;
        currentElement++;
    }
}

template<typename T>
inline bool isMemoryEqual(T* a, T* b, smm length)
{
    // Shortcut if we're comparing memory with itself
    return (a == b)
        || (memcmp(a, b, sizeof(T) * length) == 0);
}

template<typename T>
T* copyRegion(T* sourceArray, s32 sourceArrayWidth, s32 sourceArrayHeight, Rect2I region, MemoryArena* arena)
{
    ASSERT(contains(irectXYWH(0, 0, sourceArrayWidth, sourceArrayHeight), region));

    T* result = allocateMultiple<T>(arena, areaOf(region));

    T* pos = result;

    for (s32 y = region.y; y < region.y + region.h; y++) {
        // Copy whole rows at a time
        copyMemory(sourceArray + (y * sourceArrayWidth) + region.x, pos, region.w);
        pos += region.w;
    }

    return result;
}

template<typename T>
void setRegion(T* array, s32 arrayWidth, s32 arrayHeight, Rect2I region, T value)
{
    ASSERT(contains(irectXYWH(0, 0, arrayWidth, arrayHeight), region));

    for (s32 y = region.y; y < region.y + region.h; y++) {
        // Set whole rows at a time
        fillMemory<T>(array + (y * arrayWidth) + region.x, value, region.w);
    }
}
