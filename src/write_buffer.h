/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/MemoryArena.h>

/*

    A WriteBuffer lets you compose a blob of binary data, without knowing in
    advance how large it will be. The buffer is divided into uniform chunks.
    Changing the data by direct pointer access is not allowed, as the type you
    are changing may be split across multiple chunks. Instead, use overwriteAt(),
    which makes those changes correctly. Functions that append data return a
    WriteBufferLocation for the start of that data, to make using overwriteAt()
    easier.

    As of 29/04/2021, we only use this in BinaryFileWriter, which wraps it. We
    could just merge this into that, but I feel like this might be useful for
    writing network packets at some point, or possibly binary files that don't
    match our special format, so I'm keeping it for now.     - Sam

*/

struct WriteBufferChunk {
    s32 used;
    u8* bytes;

    WriteBufferChunk* nextChunk;
};

typedef s32 WriteBufferLocation;

struct WriteBufferRange {
    WriteBufferLocation start;
    s32 length;
};

struct WriteBuffer {
    MemoryArena* arena;
    s32 chunkSize;

    s32 byteCount;
    s32 chunkCount;
    WriteBufferChunk* firstChunk;
    WriteBufferChunk* lastChunk;

    // Methods

    void init(s32 chunkSize = KB(4), MemoryArena* arena = &temp_arena());

    template<typename T>
    WriteBufferRange append(T* thing)
    {
        return appendBytes(sizeof(T), thing);
    }

    template<typename T>
    WriteBufferRange reserve()
    {
        return reserveBytes(sizeof(T));
    }

    WriteBufferRange appendBytes(s32 length, void* bytes);
    WriteBufferRange reserveBytes(s32 length);

    WriteBufferLocation getCurrentPosition();
    s32 getLengthSince(WriteBufferLocation start); // How many bytes were output since that point

    template<typename T>
    T readAt(WriteBufferRange range)
    {
        ASSERT(sizeof(T) <= range.length);
        return readAt<T>(range.start);
    }

    template<typename T>
    T readAt(WriteBufferLocation location)
    {
        // Make sure the requested range is valid
        ASSERT((location + sizeof(T)) <= byteCount);

        T result;

        WriteBufferChunk* chunk = getChunkAt(location);
        s32 posInChunk = location % chunkSize;

        u8* dataPos = (u8*)&result;
        s32 remainingLength = sizeof(T);
        while (remainingLength > 0) {
            s32 remainingInChunk = chunkSize - posInChunk;
            if (remainingInChunk > remainingLength) {
                // Copy the whole thing into the current chunk
                copyMemory((chunk->bytes + posInChunk), dataPos, remainingLength);
                remainingLength = 0;
            } else {
                // Copy the amount that will fit in the current chunk
                s32 lengthToCopy = remainingInChunk;
                copyMemory((chunk->bytes + posInChunk), dataPos, lengthToCopy);
                dataPos += lengthToCopy;
                remainingLength -= lengthToCopy;

                // Go to next chunk
                chunk = chunk->nextChunk;
                posInChunk = 0;
            }
        }

        return result;
    }

    void overwriteAt(WriteBufferLocation location, s32 length, void* data);

    template<typename T>
    void overwriteAt(WriteBufferRange range, T* data)
    {
        ASSERT(sizeof(T) <= range.length);
        overwriteAt(range.start, min(range.length, (s32)sizeof(T)), data);
    }

    bool writeToFile(struct FileHandle* file);

    // Internal

    WriteBufferChunk* getChunkAt(WriteBufferLocation location);
    void appendNewChunk();
};
