/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Gfx/Forward.h>
#include <Util/Pool.h>
#include <Util/String.h>

struct RenderBufferChunk : PoolItem {
    smm size;
    smm used;
    u8* memory;

    // TODO: @Size: Could potentially re-use the pointers from the PoolItem, if we wanted.
    RenderBufferChunk* prevChunk;
    RenderBufferChunk* nextChunk;
};

struct RenderBuffer : PoolItem {
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
