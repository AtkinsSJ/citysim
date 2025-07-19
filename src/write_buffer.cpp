/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "write_buffer.h"
#include "file.h"
#include <Util/MemoryArena.h>

void WriteBuffer::init(s32 chunkSize_, MemoryArena* arena_)
{
    this->arena = arena_;
    this->chunkSize = chunkSize_;
    this->byteCount = 0;
    this->chunkCount = 0;
    this->firstChunk = nullptr;
    this->lastChunk = nullptr;

    appendNewChunk();
}

WriteBufferRange WriteBuffer::appendBytes(s32 length, void* bytes)
{
    WriteBufferRange result = {};
    result.start = getCurrentPosition();
    result.length = length;

    // Data might not fit in the current chunk, so keep allocating new ones until we're done
    s32 remainingLength = length;
    u8* dataPos = (u8*)bytes;

    while (remainingLength > 0) {
        s32 remainingInChunk = chunkSize - lastChunk->used;
        if (remainingInChunk > remainingLength) {
            // Copy the whole thing into the current chunk
            copyMemory(dataPos, (lastChunk->bytes + lastChunk->used), remainingLength);
            lastChunk->used += remainingLength;
            byteCount += remainingLength;
            remainingLength = 0;
        } else {
            // Copy the amount that will fit in the current chunk
            s32 lengthToCopy = remainingInChunk;
            copyMemory(dataPos, (lastChunk->bytes + lastChunk->used), lengthToCopy);
            dataPos += lengthToCopy;
            lastChunk->used += lengthToCopy;
            byteCount += lengthToCopy;
            remainingLength -= lengthToCopy;

            // Get a new chunk
            appendNewChunk();
        }
    }

    return result;
}

WriteBufferRange WriteBuffer::reserveBytes(s32 length)
{
    WriteBufferRange result = {};
    result.start = getCurrentPosition();
    result.length = length;

    s32 remainingLength = length;

    while (remainingLength > 0) {
        s32 remainingInChunk = chunkSize - lastChunk->used;
        if (remainingInChunk > remainingLength) {
            // Fit the whole thing into the current chunk
            lastChunk->used += remainingLength;
            byteCount += remainingLength;
            remainingLength = 0;
        } else {
            // Fit the amount that will fit in the current chunk
            s32 lengthToCopy = remainingInChunk;
            lastChunk->used += lengthToCopy;
            byteCount += lengthToCopy;
            remainingLength -= lengthToCopy;

            // Get a new chunk
            appendNewChunk();
        }
    }

    return result;
}

WriteBufferLocation WriteBuffer::getCurrentPosition()
{
    return byteCount;
}

s32 WriteBuffer::getLengthSince(WriteBufferLocation start)
{
    return getCurrentPosition() - start;
}

void WriteBuffer::overwriteAt(WriteBufferLocation location, s32 length, void* data)
{
    // Make sure the requested range is valid
    ASSERT((location + length) <= byteCount);

    // Find the chunk this starts in
    WriteBufferChunk* chunk = getChunkAt(location);
    s32 posInChunk = location % chunkSize;

    s32 remainingLength = length;
    u8* dataPos = (u8*)data;
    while (remainingLength > 0) {
        s32 remainingInChunk = chunkSize - posInChunk;
        if (remainingInChunk > remainingLength) {
            // Copy the whole thing into the current chunk
            copyMemory(dataPos, (chunk->bytes + posInChunk), remainingLength);
            remainingLength = 0;
        } else {
            // Copy the amount that will fit in the current chunk
            s32 lengthToCopy = remainingInChunk;
            copyMemory(dataPos, (chunk->bytes + posInChunk), lengthToCopy);
            dataPos += lengthToCopy;
            remainingLength -= lengthToCopy;

            // Go to next chunk
            chunk = chunk->nextChunk;
            posInChunk = 0;
        }
    }
}

bool WriteBuffer::writeToFile(FileHandle* file)
{
    bool succeeded = true;

    // Iterate chunks
    for (WriteBufferChunk* chunk = firstChunk;
        chunk != nullptr;
        chunk = chunk->nextChunk) {
        succeeded = ::writeToFile(file, chunk->used, chunk->bytes);
        if (!succeeded)
            break;
    }

    return succeeded;
}

WriteBufferChunk* WriteBuffer::getChunkAt(WriteBufferLocation location)
{
    s32 chunkIndex = location / chunkSize;
    WriteBufferChunk* chunk = firstChunk;
    while (chunkIndex > 0) {
        chunk = chunk->nextChunk;
        chunkIndex--;
    }

    return chunk;
}

void WriteBuffer::appendNewChunk()
{
    Blob blob = arena->allocate_blob(sizeof(WriteBufferChunk) + chunkSize);
    WriteBufferChunk* newChunk = (WriteBufferChunk*)blob.data();
    newChunk->used = 0;
    newChunk->bytes = blob.writable_data() + sizeof(WriteBufferChunk);
    newChunk->nextChunk = nullptr;

    if (chunkCount == 0) {
        firstChunk = lastChunk = newChunk;
        chunkCount = 1;
    } else {
        lastChunk->nextChunk = newChunk;
        lastChunk = newChunk;
        chunkCount++;
    }
}
