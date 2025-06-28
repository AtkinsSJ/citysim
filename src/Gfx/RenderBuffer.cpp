/*
 * Copyright (c) 2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Gfx/RenderBuffer.h>
#include <Gfx/Renderer.h>

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

void RenderBuffer::clear_for_pool()
{
    currentShader = -1;
    currentTexture = nullptr;

    for (RenderBufferChunk* chunk = firstChunk;
        chunk != nullptr;
        chunk = chunk->nextChunk) {
        chunk->used = 0;
        addItemToPool(chunkPool, chunk);
    }

    firstChunk = nullptr;
    currentChunk = nullptr;
}
