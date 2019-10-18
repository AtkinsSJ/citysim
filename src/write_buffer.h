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
template<typename T>
inline void appendStruct(WriteBuffer *buffer, T *data)
{
	append(buffer, sizeof(T), data);
}

void appendS8(WriteBuffer *buffer, s8 byte);
void appendU8(WriteBuffer *buffer, u8 byte);

// Returns the number of bytes written
s32 appendRLE(WriteBuffer *buffer, s32 length, u8 *data);

// Like append(), but leaves the bytes blank to fill in later. Returns the start byte's index.
s32 reserve(WriteBuffer *buffer, s32 length);

s32 getCurrentPosition(WriteBuffer *buffer);

void overwriteAt(WriteBuffer *buffer, s32 indexInBuffer, s32 length, void *data);

// Internal
void appendNewChunk(WriteBuffer *buffer);
