/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Allocator.h>
#include <Util/Basic.h>
#include <Util/Memory.h>
#include <Util/Optional.h>
#include <Util/String.h>

class MemoryArena final : public Allocator {
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

private:
    bool allocate_block(size_t size);
    void free_current_block();

    virtual Span<u8> allocate_internal(size_t size) override;

    String m_name { "UNINITIALIZED"_s };
    MemoryBlock* m_current_block { nullptr };
    size_t m_minimum_block_size { 0 };
    size_t m_external_tracked_memory_size { 0 };
    ResetState m_reset_state {};
};

MemoryArena& temp_arena();
