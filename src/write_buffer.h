#pragma once

/*

	A WriteBuffer lets you compose a blob of binary data, without knowing in
	advance how large it will be. The buffer is divided into uniform chunks.
	Changing the data by direct pointer access is not allowed, as the type you
	are changing may be split across multiple chunks. Instead, use overwriteAt(),
	which makes those changes correctly. Functions that append data return a
	WriteBufferLocation for the start of that data, to make using overwriteAt()
	easier.

*/

struct WriteBufferChunk
{
	s32 used;
	u8 *bytes;

	WriteBufferChunk *nextChunk;
};

typedef s32 WriteBufferLocation;
struct WriteBufferRange
{
	WriteBufferLocation start;
	s32 length;
};

struct WriteBuffer
{
	MemoryArena *arena;
	s32 chunkSize;

	s32 byteCount;
	s32 chunkCount;
	WriteBufferChunk *firstChunk;
	WriteBufferChunk *lastChunk;

// Methods

	void init(s32 chunkSize = KB(4), MemoryArena *arena = tempArena);

	template <typename T>
	WriteBufferLocation appendLiteral(T literal);
	template <typename T>
	WriteBufferLocation reserveLiteral();

	template <typename T>
	WriteBufferLocation appendStruct(T *theStruct);
	template <typename T>
	WriteBufferLocation reserveStruct();

	WriteBufferLocation appendBytes(s32 length, void *bytes);
	WriteBufferLocation reserveBytes(s32 length);

	WriteBufferRange appendRLEBytes(s32 length, u8 *bytes);

	WriteBufferLocation getCurrentPosition();
	s32 getLengthSince(WriteBufferLocation start); // How many bytes were output since that point

	void overwriteAt(WriteBufferLocation location, s32 length, void *data);

	bool writeToFile(FileHandle *file);


// Internal

	void appendNewChunk();
};
