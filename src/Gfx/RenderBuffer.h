/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Gfx/RenderItem.h>
#include <Util/Pool.h>
#include <Util/String.h>

struct RenderBufferChunk final : public Poolable<RenderBufferChunk> {
    RenderBufferChunk() = default;

    // Poolable
    static RenderBufferChunk& allocate_from_pool(MemoryArena&);
    void initialize_from_pool() { }
    virtual void clear_for_pool() override;

    static constexpr u64 DEFAULT_SIZE = KB(64);

    smm size;
    smm used;
    u8* memory;

    // TODO: @Size: Could potentially re-use the pointers from the PoolItem, if we wanted.
    RenderBufferChunk* prevChunk;
    RenderBufferChunk* nextChunk;
};

struct RenderBuffer final : public Poolable<RenderBuffer> {
    RenderBuffer() = default;
    void take_from(RenderBuffer& other);

    // Poolable
    void initialize_from_pool(MemoryArena&, String name, Pool<RenderBufferChunk>*);
    virtual void clear_for_pool() override;

    String name;

    RenderBufferChunk* firstChunk;
    RenderBufferChunk* currentChunk;
    Pool<RenderBufferChunk>* chunkPool;

    // Transient stuff
    bool hasRangeReserved;
    s32 scissorCount;
    s8 currentShader;
    Asset* currentTexture;
};
