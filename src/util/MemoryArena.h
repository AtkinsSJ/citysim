/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Assert.h"
#include "Basic.h"
#include "String.h"

// NB: MemoryBlock is positioned just before its memory pointer.
// So when deallocating, we can just free(block)!
struct MemoryBlock {
    MemoryBlock* prevBlock;

    smm size;
    smm used;
    u8* memory;
};

struct MemoryArenaResetState {
    MemoryBlock* currentBlock;
    smm used;
};

struct MemoryArena {
    String name;

    MemoryBlock* currentBlock;

    smm minimumBlockSize;

    MemoryArenaResetState resetState;
};

MemoryArena& temp_arena();

// Creates an arena, and pushes a struct on it which contains the arena.
// Optional final param is the block size
#define bootstrapArena(containerType, containerName, arenaVarName, ...)                                                    \
    {                                                                                                                      \
        MemoryArena bootstrap;                                                                                             \
        /* Hack: Detect if the block-size was provided, and default to MB(1) */                                            \
        smm minimumBlockSize = (__VA_ARGS__ - 0) > 0 ? __VA_ARGS__ : MB(1);                                                \
        bool bootstrapSucceeded = initMemoryArena(&bootstrap, #arenaVarName##_s, sizeof(containerType), minimumBlockSize); \
        ASSERT(bootstrapSucceeded);                                                                                        \
        containerName = allocateStruct<containerType>(&bootstrap);                                                         \
        containerName->arenaVarName = bootstrap;                                                                           \
        markResetPosition(&containerName->arenaVarName);                                                                   \
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
        T *result = nullptr;

        MemoryArena bootstrap;
        initMemoryArena(&bootstrap, sizeof(T));
        result = allocateStruct<T>(&bootstrap);

        MemoryArena *arena = (MemoryArena *)((u8*)result + offsetOfArenaMember);
        *arena = bootstrap;
        markResetPosition(arena);

        return result;
}
*/

bool initMemoryArena(MemoryArena* arena, String name, smm size, smm minimumBlockSize = MB(1));
MemoryArenaResetState getArenaPosition(MemoryArena* arena);
void revertMemoryArena(MemoryArena* arena, MemoryArenaResetState resetState);
void markResetPosition(MemoryArena* arena);
void resetMemoryArena(MemoryArena* arena);
void freeMemoryArena(MemoryArena* arena);

void* allocate(MemoryArena* arena, smm size);
Blob allocateBlob(MemoryArena* arena, smm size);

template<typename T>
T* allocateStruct(MemoryArena* arena)
{
    return (T*)allocate(arena, sizeof(T));
}

template<typename T>
T* allocateMultiple(MemoryArena* arena, smm count)
{
    // Negative allocations are obviously incorrect, so we assert them.
    // However, count=0 sometimes is used, eg when interning an empty string that's used
    // as a hashtable key. To avoid needlessly complicating user code, we just return null
    // here when you try to allocate 0 things. The end result is the same - you're not going
    // to use the memory if you've got 0 things allocated anyway!
    // - Sam, 28/03/2020
    ASSERT(count >= 0);
    T* result = nullptr;

    if (count > 0) {
        result = (T*)allocate(arena, sizeof(T) * count);
    }

    return result;
}

template<typename T>
Array<T> allocateArray(MemoryArena* arena, s32 count, bool markAsFull = false)
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
Array2<T> allocateArray2(MemoryArena* arena, s32 w, s32 h)
{
    ASSERT(w > 0 && h > 0);
    return makeArray2<T>(w, h, allocateMultiple<T>(arena, w * h));
}

//
// Tools for 2D arrays
//

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
