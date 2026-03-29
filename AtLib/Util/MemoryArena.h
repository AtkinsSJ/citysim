/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
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

    Blob allocate_blob(size_t size);

    template<typename T, typename... Args>
    T* allocate(Args&&... args)
    {
        auto memory = allocate_internal(sizeof(T));
        return new (memory.raw_data()) T(forward<Args>(args)...);
    }

    template<typename T>
    Span<T> allocate_multiple(size_t count)
    {
        if (count == 0)
            return {};

        auto memory = allocate_internal(sizeof(T) * count);
        auto* items = new (memory.raw_data()) T[count];
        return Span { count, items };
    }

    template<typename T, typename Item = u8>
    struct ObjectAndData {
        T& object;
        Span<Item> data;
    };
    template<typename T, typename Item = u8>
    ObjectAndData<T, Item> allocate_with_data(size_t item_count)
    {
        // FIXME: Figure out alignment requirements so that the Item array is aligned too.
        auto memory = allocate_internal(sizeof(T) + (sizeof(Item) * item_count));
        ObjectAndData<T, Item> result {
            .object = *reinterpret_cast<T*>(memory.raw_data()),
            .data = { item_count, reinterpret_cast<Item*>(reinterpret_cast<u8*>(memory.raw_data()) + sizeof(T)) },
        };
        new (&result.object) T();
        return result;
    }

    template<typename T>
    Array<T> allocate_array(size_t count, bool mark_as_full = false)
    {
        auto items = allocate_multiple<T>(count);
        return makeArray<T>(items.size(), items.raw_data(), mark_as_full ? count : 0);
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
        return Array2<T> { w, h, allocate_multiple<T>(w * h) };
    }

    String allocate_string(StringView input);

private:
    bool allocate_block(size_t size);
    void free_current_block();

    Span<u8> allocate_internal(size_t size);

    String m_name { "UNINITIALIZED"_s };
    MemoryBlock* m_current_block { nullptr };
    size_t m_minimum_block_size { 0 };
    size_t m_external_tracked_memory_size { 0 };
    ResetState m_reset_state {};
};

MemoryArena& temp_arena();
