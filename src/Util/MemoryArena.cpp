/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Util/Log.h>
#include <Util/Maths.h>
#include <Util/Memory.h>
#include <Util/MemoryArena.h>

static MemoryArena s_temp_arena { "Temp"_s, MB(4) };

MemoryArena::MemoryArena(String name, Optional<size_t> initial_size, size_t minimum_block_size)
    : m_name(name)
    , m_minimum_block_size(minimum_block_size)
{
    if (initial_size.has_value()) {
        auto block_size = max(initial_size.value(), m_minimum_block_size);
        if (!allocate_block(block_size)) {
            logCritical("Unable to allocate {0} bytes for {1}!"_s, { formatInt(block_size), m_name });
            ASSERT(!"Failed an allocation!");
        }
    }
    mark_reset_position();
}

MemoryArena::MemoryArena(MemoryArena&& other)
    : m_name(other.m_name)
    , m_current_block(other.m_current_block)
    , m_minimum_block_size(other.m_minimum_block_size)
    , m_external_tracked_memory_size(other.m_external_tracked_memory_size)
    , m_reset_state(other.m_reset_state)
{
    other = {};
}

MemoryArena& MemoryArena::operator=(MemoryArena&& other)
{
    this->~MemoryArena();

    m_name = other.m_name;
    m_current_block = other.m_current_block;
    m_minimum_block_size = other.m_minimum_block_size;
    m_external_tracked_memory_size = other.m_external_tracked_memory_size;
    m_reset_state = other.m_reset_state;

    other.m_current_block = {};
    other.m_reset_state = {};

    return *this;
}

MemoryArena::~MemoryArena()
{
    if (m_current_block) {
        // Free all but the original block
        while (m_current_block->previous_block) {
            free_current_block();
        }

        // Free original block, which may contain the arena so we have to be careful!
        MemoryBlock* final_block = m_current_block;
        m_current_block = nullptr;
        deallocateRaw(final_block);
    }
}

MemoryArena::Statistics MemoryArena::get_statistics() const
{
    Statistics statistics {};
    if (m_current_block) {
        statistics.block_count = 1;
        statistics.total_size = m_current_block->size + m_external_tracked_memory_size;
        statistics.used_size = m_current_block->used + m_external_tracked_memory_size;

        MemoryBlock const* block = m_current_block->previous_block;
        while (block) {
            statistics.block_count++;
            statistics.total_size += block->size;
            statistics.used_size += block->size;

            block = block->previous_block;
        }
    }
    return statistics;
}

MemoryArena::ResetState MemoryArena::get_current_position() const
{
    return ResetState {
        m_current_block,
        m_current_block ? m_current_block->used : 0,
    };
}

void MemoryArena::mark_reset_position()
{
    m_reset_state = get_current_position();
}

void MemoryArena::revert_to(ResetState const& reset_state)
{
    while (m_current_block != reset_state.current_block()) {
        free_current_block();
    }

    if (m_current_block) {
#if BUILD_DEBUG
        // Clear memory so we spot bugs in keeping pointers to deallocated memory.
        fillMemory<u8>(m_current_block->memory + reset_state.used(), 0xcd, m_current_block->used - reset_state.used());
#endif
        m_current_block->used = reset_state.used();
    }
}

void MemoryArena::reset()
{
    revert_to(m_reset_state);
}

Blob MemoryArena::allocate_blob(size_t size)
{
    return Blob { size, static_cast<u8*>(allocate_internal(size)) };
}

bool MemoryArena::allocate_block(size_t size)
{
    size_t totalSize = size + sizeof(MemoryBlock);
    u8* memory = allocateRaw(totalSize);

    if (!memory)
        return false;

    MemoryBlock* block = reinterpret_cast<MemoryBlock*>(memory);
    block->memory = memory + sizeof(MemoryBlock);
    block->used = 0;
    block->size = size;

    block->previous_block = m_current_block;
    m_current_block = block;

    return true;
}

void MemoryArena::free_current_block()
{
    MemoryBlock* block = m_current_block;
    ASSERT(block != nullptr); // Attempting to free non-existent block
    m_current_block = block->previous_block;
    deallocateRaw(block);
}

MemoryArena& temp_arena()
{
    return s_temp_arena;
}

void* MemoryArena::allocate_internal(size_t size)
{
    if (size > m_minimum_block_size) {
        logWarn("Large allocation in {0} arena: {1} bytes when block size is {2} bytes"_s, { name(), formatInt(size), formatInt(m_minimum_block_size) });
    }

    ASSERT(size < GB(1)); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    if ((m_current_block == 0)
        || (m_current_block->used + size > m_current_block->size)) {
        size_t new_block_size = max(size, m_minimum_block_size);
        allocate_block(new_block_size);
    }

    ASSERT(m_current_block != nullptr); // No memory in arena!

    u8* result = m_current_block->memory + m_current_block->used;
    memset(result, 0, size);

    m_current_block->used += size;

    return result;
}
