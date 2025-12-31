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

class MemoryArena {
public:
    // NB: MemoryBlock is positioned just before its memory pointer.
    // So when deallocating, we can just free(block)!
    struct MemoryBlock {
        size_t size;
        size_t used;
        MemoryBlock* previous_block;
        u8* memory;
    };

    class ResetState {
    public:
        ResetState() = default;
        explicit ResetState(MemoryBlock* current_block, size_t used)
            : m_current_block(current_block)
            , m_used(used)
        {
        }

        MemoryBlock* current_block() const { return m_current_block; }
        size_t used() const { return m_used; }

    private:
        MemoryBlock* m_current_block { nullptr };
        size_t m_used { 0 };
    };

    MemoryArena() = default;
    MemoryArena(String name, Optional<size_t> initial_size = {}, size_t minimum_block_size = MB(1));
    MemoryArena(MemoryArena&&);

    // Allocates a T, which is contained within its own MemoryArena member.
    template<typename T>
    static T* bootstrap(String name, size_t minimum_block_size = MB(1))
    {
        MemoryArena arena { name, sizeof(T), minimum_block_size };
        auto* container = arena.allocate<T>();
        container->arena = move(arena);
        container->arena.mark_reset_position();
        return container;
    }

    MemoryArena& operator=(MemoryArena&&);

    ~MemoryArena();

    String const& name() const { return m_name; }

    struct Statistics {
        size_t block_count { 0 };
        size_t used_size { 0 };
        size_t total_size { 0 };
    };
    Statistics get_statistics() const;

    ResetState get_current_position() const;
    void mark_reset_position();
    void revert_to(ResetState const&);
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
    Array2<T> allocate_array_2d(V2I size)
    {
        return allocate_array_2d<T>(size.x, size.y);
    }

    template<typename T>
    Array2<T> allocate_array_2d(u32 w, u32 h)
    {
        ASSERT(w > 0 && h > 0);
        return makeArray2<T>(w, h, allocate_multiple<T>(w * h));
    }

    String allocate_string(StringView input);

private:
    bool allocate_block(size_t size);
    void free_current_block();

    void* allocate_internal(size_t size);

    String m_name { "UNINITIALIZED"_s };
    MemoryBlock* m_current_block { nullptr };
    size_t m_minimum_block_size { 0 };
    size_t m_external_tracked_memory_size { 0 };
    ResetState m_reset_state {};
};

MemoryArena& temp_arena();
