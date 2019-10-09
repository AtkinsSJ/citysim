#pragma once

struct WriteBufferChunk
{
	s32 used;
	u8 *bytes;

	WriteBufferChunk *nextChunk;
};

struct WriteBuffer
{
	MemoryArena *arena;
	s32 chunkSize;

	s32 byteCount;
	s32 chunkCount;
	WriteBufferChunk *firstChunk;
	WriteBufferChunk *lastChunk;
};

void initWriteBuffer(WriteBuffer *buffer, s32 chunkSize = KB(4), MemoryArena *arena = tempArena);

bool writeToFile(FileHandle *file, WriteBuffer *buffer);

void append(WriteBuffer *buffer, s32 length, void *data);
// Like append(), but leaves the bytes blank to fill in later. Returns the start byte's index.
s32 reserve(WriteBuffer *buffer, s32 length);

// Internal

WriteBufferChunk *allocateWriteBufferChunk(WriteBuffer *buffer);
