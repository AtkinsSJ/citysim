/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Array2.h>
#include <Util/Assert.h>
#include <Util/Basic.h>
#include <Util/Memory.h>
#include <Util/Optional.h>
#include <Util/String.h>

// NB: MemoryBlock is positioned just before its memory pointer.
// So when deallocating, we can just free(block)!
struct MemoryBlock {
    MemoryBlock* prevBlock;

    smm size;
    smm used;
    u8* memory;
};

struct MemoryArenaResetState {
    MemoryBlock* currentBlock { nullptr };
    smm used { 0 };
};

class MemoryArena {
public:
    MemoryArena() = default;
    MemoryArena(String name, Optional<size_t> initial_size = {}, size_t minimum_block_size = MB(1));
    MemoryArena(MemoryArena&&);

    MemoryArena& operator=(MemoryArena&&);

    ~MemoryArena();

    String const& name() const { return m_name; }

    struct Statistics {
        size_t block_count { 0 };
        size_t used_size { 0 };
        size_t total_size { 0 };
    };
    Statistics get_statistics() const;

    MemoryArenaResetState get_current_position() const;
    void mark_reset_position();
    void revert_to(MemoryArenaResetState const&);
    void reset();

    void* allocate_deprecated(size_t size) { return allocate_internal(size); }
    Blob allocate_blob(size_t size);

    template<typename T, typename... Args>
    T* allocate(Args&&... args)
    {
        auto* memory = allocate_internal(sizeof(T));
        return new (memory) T(forward<Args>(args)...);
    }

    // FIXME: Return something that bounds-checks access.
    template<typename T>
    T* allocate_multiple(size_t count)
    {
        // Negative allocations are obviously incorrect, so we assert them.
        // However, count=0 sometimes is used, eg when interning an empty string that's used
        // as a hashtable key. To avoid needlessly complicating user code, we just return null
        // here when you try to allocate 0 things. The end result is the same - you're not going
        // to use the memory if you've got 0 things allocated anyway!
        // - Sam, 28/03/2020
        ASSERT(count >= 0);

        if (count == 0)
            return nullptr;

        auto* memory = allocate_internal(sizeof(T) * count);
        return new (memory) T[count];
    }

    template<typename T>
    Array<T> allocate_array(s32 count, bool markAsFull = false)
    {
        Array<T> result;

        if (count == 0) {
            result = makeEmptyArray<T>();
        } else {
            ASSERT(count > 0);
            result = makeArray<T>(count, allocate_multiple<T>(count), markAsFull ? count : 0);
        }

        return result;
    }

    template<typename T>
    Array2<T> allocate_array_2d(u32 w, u32 h)
    {
        ASSERT(w > 0 && h > 0);
        return makeArray2<T>(w, h, allocate_multiple<T>(w * h));
    }

private:
    bool allocate_block(size_t size);
    void free_current_block();

    void* allocate_internal(size_t size);

    String m_name { "UNINITIALIZED"_s };
    MemoryBlock* m_current_block { nullptr };
    size_t m_minimum_block_size { 0 };
    size_t m_external_tracked_memory_size { 0 };
    MemoryArenaResetState m_reset_state {};
};

MemoryArena& temp_arena();

// Creates an arena, and pushes a struct on it which contains the arena.
// Optional final param is the block size
#define bootstrapArena(containerType, containerName, arenaVarName, ...)                       \
    {                                                                                         \
        /* Hack: Detect if the block-size was provided, and default to MB(1) */               \
        size_t minimumBlockSize = (__VA_ARGS__ - 0) > 0 ? __VA_ARGS__ : MB(1);                \
        MemoryArena bootstrap { #arenaVarName##_s, sizeof(containerType), minimumBlockSize }; \
        containerName = bootstrap.allocate<containerType>();                                  \
        containerName->arenaVarName = move(bootstrap);                                        \
        containerName->arenaVarName.mark_reset_position();                                    \
    }

//
// Tools for 2D arrays
//

template<typename T>
T* copyRegion(T* sourceArray, s32 sourceArrayWidth, s32 sourceArrayHeight, Rect2I region, MemoryArena* arena)
{
    ASSERT(contains(irectXYWH(0, 0, sourceArrayWidth, sourceArrayHeight), region));

    T* result = arena->allocate_multiple<T>(areaOf(region));

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
