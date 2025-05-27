#pragma once

void WriteBuffer::init(s32 chunkSize_, MemoryArena* arena_)
{
    this->arena = arena_;
    this->chunkSize = chunkSize_;
    this->byteCount = 0;
    this->chunkCount = 0;
    this->firstChunk = null;
    this->lastChunk = null;

    appendNewChunk();
}

template<typename T>
inline WriteBufferRange WriteBuffer::append(T* thing)
{
    return appendBytes(sizeof(T), thing);
}

template<typename T>
inline WriteBufferRange WriteBuffer::reserve()
{
    return reserveBytes(sizeof(T));
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

inline WriteBufferLocation WriteBuffer::getCurrentPosition()
{
    return byteCount;
}

s32 WriteBuffer::getLengthSince(WriteBufferLocation start)
{
    return getCurrentPosition() - start;
}

template<typename T>
T WriteBuffer::readAt(WriteBufferRange range)
{
    ASSERT(sizeof(T) <= range.length);
    return readAt<T>(range.start);
}

template<typename T>
T WriteBuffer::readAt(WriteBufferLocation location)
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

template<typename T>
void WriteBuffer::overwriteAt(WriteBufferRange range, T* data)
{
    ASSERT(sizeof(T) <= range.length);
    overwriteAt(range.start, min(range.length, (s32)sizeof(T)), data);
}

bool WriteBuffer::writeToFile(FileHandle* file)
{
    bool succeeded = true;

    // Iterate chunks
    for (WriteBufferChunk* chunk = firstChunk;
        chunk != null;
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
    Blob blob = allocateBlob(arena, sizeof(WriteBufferChunk) + chunkSize);
    WriteBufferChunk* newChunk = (WriteBufferChunk*)blob.memory;
    newChunk->used = 0;
    newChunk->bytes = (u8*)(blob.memory + sizeof(WriteBufferChunk));
    newChunk->nextChunk = null;

    if (chunkCount == 0) {
        firstChunk = lastChunk = newChunk;
        chunkCount = 1;
    } else {
        lastChunk->nextChunk = newChunk;
        lastChunk = newChunk;
        chunkCount++;
    }
}
