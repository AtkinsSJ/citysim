#pragma once

void WriteBuffer::init(s32 chunkSize_, MemoryArena *arena_)
{
	this->arena = arena_;
	this->chunkSize = chunkSize_;
	this->byteCount = 0;
	this->chunkCount = 0;
	this->firstChunk = null;
	this->lastChunk = null;

	appendNewChunk();
}

template <typename T>
inline WriteBufferLocation WriteBuffer::appendLiteral(T literal)
{
	return appendBytes(sizeof(T), &literal);
}

template <typename T>
inline WriteBufferLocation WriteBuffer::reserveLiteral()
{
	return reserveBytes(sizeof(T));
}

template <typename T>
inline WriteBufferLocation WriteBuffer::appendStruct(T *theStruct)
{
	return appendBytes(sizeof(T), theStruct);
}

template <typename T>
inline WriteBufferLocation WriteBuffer::reserveStruct()
{
	return reserveBytes(sizeof(T));
}

WriteBufferLocation WriteBuffer::appendBytes(s32 length, void *bytes)
{
	WriteBufferLocation startLoc = getCurrentPosition();

	// Data might not fit in the current chunk, so keep allocating new ones until we're done
	s32 remainingLength = length;
	u8 *dataPos = (u8*) bytes;

	while (remainingLength > 0)
	{
		s32 remainingInChunk = chunkSize - lastChunk->used;
		if (remainingInChunk > remainingLength)
		{
			// Copy the whole thing into the current chunk
			copyMemory(dataPos, (lastChunk->bytes + lastChunk->used), remainingLength);
			lastChunk->used += remainingLength;
			byteCount += remainingLength;
			remainingLength = 0;
		}
		else
		{
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

	return startLoc;
}

WriteBufferLocation WriteBuffer::reserveBytes(s32 length)
{
	WriteBufferLocation result = getCurrentPosition();

	s32 remainingLength = length;

	while (remainingLength > 0)
	{
		s32 remainingInChunk = chunkSize - lastChunk->used;
		if (remainingInChunk > remainingLength)
		{
			// Fit the whole thing into the current chunk
			lastChunk->used += remainingLength;
			byteCount += remainingLength;
			remainingLength = 0;
		}
		else
		{
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

WriteBufferRange WriteBuffer::appendRLEBytes(s32 length, u8 *bytes)
{
	WriteBufferRange result = {};
	result.start = getCurrentPosition();

	// Our scheme is, (s8 length, u8...data)
	// Positive length = repeat the next byte `length` times.
	// Negative length = copy the next `-length` bytes literally.

	// const s32 minRunLength = 4; // This is a fairly arbitrary number! Maybe it should be bigger, idk.

	// Though, for attempt #1 we'll stick to always outputting runs, even if it's
	// a run of length 1.
	u8 *end = bytes + length;
	u8 *pos = bytes;

	while (pos < end)
	{
		s8 count = 1;
		u8 value = *pos++;
		while ((pos < end) && (count < s8Max) && (*pos == value))
		{
			count++;
			pos++;
		}

		appendLiteral<s8>(count);
		appendLiteral<u8>(value);
		result.length += 2;
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

void WriteBuffer::overwriteAt(WriteBufferLocation location, s32 length, void *data)
{
	// Find the chunk this starts in
	WriteBufferChunk *chunk;
	s32 chunkIndex = location / chunkSize;
	chunk = firstChunk;
	while (chunkIndex > 0)
	{
		chunk = chunk->nextChunk;
		chunkIndex--;
	}

	s32 posInChunk = location % chunkSize;

	s32 remainingLength = length;
	u8 *dataPos = (u8*) data;
	while (remainingLength > 0)
	{
		s32 remainingInChunk = chunkSize - posInChunk;
		if (remainingInChunk > remainingLength)
		{
			// Copy the whole thing into the current chunk
			copyMemory(dataPos, (chunk->bytes + posInChunk), remainingLength);
			remainingLength = 0;
		}
		else
		{
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

bool WriteBuffer::writeToFile(FileHandle *file)
{
	bool succeeded = true;

	// Iterate chunks
	for (WriteBufferChunk *chunk = firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		succeeded = ::writeToFile(file, chunk->used, chunk->bytes);
		if (!succeeded) break;
	}

	return succeeded;
}

void WriteBuffer::appendNewChunk()
{
	Blob blob = allocateBlob(arena, sizeof(WriteBufferChunk) + chunkSize);
	WriteBufferChunk *newChunk = (WriteBufferChunk *)blob.memory;
	newChunk->used = 0;
	newChunk->bytes = (u8 *) (blob.memory + sizeof(WriteBufferChunk));
	newChunk->nextChunk = null;

	if (chunkCount == 0)
	{
		firstChunk = lastChunk = newChunk;
		chunkCount = 1;
	}
	else
	{
		lastChunk->nextChunk = newChunk;
		lastChunk = newChunk;
		chunkCount++;
	}
}
