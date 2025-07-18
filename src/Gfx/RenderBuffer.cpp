/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Gfx/RenderBuffer.h>
#include <Gfx/Renderer.h>

RenderBufferChunk& RenderBufferChunk::allocate_from_pool(MemoryArena& arena)
{
    RenderBufferChunk* result = static_cast<RenderBufferChunk*>(allocate(&arena, DEFAULT_SIZE + sizeof(RenderBufferChunk)));
    result->size = DEFAULT_SIZE;
    result->used = 0;
    result->memory = reinterpret_cast<u8*>(result + 1);

    return *result;
}

void RenderBufferChunk::clear_for_pool()
{
    used = 0;
    prevChunk = nullptr;
    nextChunk = nullptr;
}

void RenderBuffer::take_from(RenderBuffer& other)
{
    if (other.currentChunk != nullptr) {
        // If the target is empty, we can take a shortcut and just copy the pointers
        if (this->currentChunk == nullptr) {
            this->firstChunk = other.firstChunk;
            this->currentChunk = other.currentChunk;
        } else {
            ASSERT((this->currentChunk->size - this->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            appendRenderItemType(this, RenderItemType_NextMemoryChunk);
            this->currentChunk->nextChunk = other.firstChunk;
            other.firstChunk->prevChunk = this->currentChunk;
            this->currentChunk = other.currentChunk;
        }
    }

    other.firstChunk = nullptr;
    other.currentChunk = nullptr;
    other.clear_for_pool(); // Make sure things are reset
}

void RenderBuffer::initialize_from_pool(MemoryArena& arena, String name, Pool<RenderBufferChunk>* chunk_pool)
{
    this->name = pushString(&arena, name);
    hashString(&this->name);

    this->hasRangeReserved = false;
    this->scissorCount = 0;

    this->chunkPool = chunk_pool;

    this->firstChunk = nullptr;
    this->currentChunk = nullptr;

    this->currentShader = -1;
    this->currentTexture = nullptr;
}

void RenderBuffer::clear_for_pool()
{
    currentShader = -1;
    currentTexture = nullptr;

    for (RenderBufferChunk* chunk = firstChunk; chunk != nullptr;) {
        // NB: `discard()` wipes the nextChunk member, so we need to grab it first.
        auto* next_chunk = chunk->nextChunk;
        chunkPool->discard(*chunk);
        chunk = next_chunk;
    }

    firstChunk = nullptr;
    currentChunk = nullptr;
}
