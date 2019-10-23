#pragma once

void initWriteBuffer(WriteBuffer *buffer, s32 chunkSize, MemoryArena *arena)
{
	buffer->arena = arena;
	buffer->chunkSize = chunkSize;
	buffer->byteCount = 0;
	buffer->chunkCount = 0;
	buffer->firstChunk = null;
	buffer->lastChunk = null;

	appendNewChunk(buffer);
}

bool writeToFile(FileHandle *file, WriteBuffer *buffer)
{
	bool succeeded = true;

	// Iterate chunks
	for (WriteBufferChunk *chunk = buffer->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		succeeded = writeToFile(file, chunk->used, chunk->bytes);
		if (!succeeded) break;
	}

	return succeeded;
}

void appendNewChunk(WriteBuffer *buffer)
{
	Blob blob = allocateBlob(buffer->arena, sizeof(WriteBufferChunk) + buffer->chunkSize);
	WriteBufferChunk *newChunk = (WriteBufferChunk *)blob.memory;
	newChunk->used = 0;
	newChunk->bytes = (u8 *) (blob.memory + sizeof(WriteBufferChunk));
	newChunk->nextChunk = null;

	if (buffer->chunkCount == 0)
	{
		buffer->firstChunk = buffer->lastChunk = newChunk;
		buffer->chunkCount = 1;
	}
	else
	{
		buffer->lastChunk->nextChunk = newChunk;
		buffer->lastChunk = newChunk;
		buffer->chunkCount++;
	}
}

void appendS8(WriteBuffer *buffer, s8 byte)
{
	if (buffer->lastChunk->used >= buffer->chunkSize)
	{
		appendNewChunk(buffer);
	}

	buffer->lastChunk->bytes[buffer->lastChunk->used++] = *((u8*)&byte);
	buffer->byteCount++;
}

void appendU8(WriteBuffer *buffer, u8 byte)
{
	if (buffer->lastChunk->used >= buffer->chunkSize)
	{
		appendNewChunk(buffer);
	}

	buffer->lastChunk->bytes[buffer->lastChunk->used++] = byte;
	buffer->byteCount++;
}

void append(WriteBuffer *buffer, s32 length, void *data)
{
	// Data might not fit in the current chunk, so keep allocating new ones until we're done
	s32 remainingLength = length;
	u8 *dataPos = (u8*) data;

	while (remainingLength > 0)
	{
		s32 remainingInChunk = buffer->chunkSize - buffer->lastChunk->used;
		if (remainingInChunk > remainingLength)
		{
			// Copy the whole thing into the current chunk
			copyMemory(dataPos, (buffer->lastChunk->bytes + buffer->lastChunk->used), remainingLength);
			buffer->lastChunk->used += remainingLength;
			buffer->byteCount += remainingLength;
			remainingLength = 0;
		}
		else
		{
			// Copy the amount that will fit in the current chunk
			s32 lengthToCopy = remainingInChunk;
			copyMemory(dataPos, (buffer->lastChunk->bytes + buffer->lastChunk->used), lengthToCopy);
			dataPos += lengthToCopy;
			buffer->lastChunk->used += lengthToCopy;
			buffer->byteCount += lengthToCopy;
			remainingLength -= lengthToCopy;

			// Get a new chunk
			appendNewChunk(buffer);
		}
	}
}

s32 appendRLE(WriteBuffer *buffer, s32 length, u8 *data)
{
	// Our scheme is, (s8 length, u8...data)
	// Positive length = repeat the next byte `length` times.
	// Negative length = copy the next `-length` bytes literally.

	// const s32 minRunLength = 4; // This is a fairly arbitrary number! Maybe it should be bigger, idk.

	// Though, for attempt #1 we'll stick to always outputting runs, even if it's
	// a run of length 1.
	u8 *end = data + length;
	u8 *pos = data;

	s32 outputLength = 0;

	while (pos < end)
	{
		s8 count = 1;
		u8 value = *pos++;
		while ((pos < end) && (count < s8Max) && (*pos == value))
		{
			count++;
			pos++;
		}

		appendS8(buffer, count);
		appendU8(buffer, value);
		outputLength += 2;
	}

	return outputLength;
}

s32 getCurrentPosition(WriteBuffer *buffer)
{
	s32 result = buffer->byteCount;
	return result;
}

s32 reserve(WriteBuffer *buffer, s32 length)
{
	s32 result = buffer->byteCount;

	s32 remainingLength = length;

	while (remainingLength > 0)
	{
		s32 remainingInChunk = buffer->chunkSize - buffer->lastChunk->used;
		if (remainingInChunk > remainingLength)
		{
			// Fit the whole thing into the current chunk
			buffer->lastChunk->used += remainingLength;
			buffer->byteCount += remainingLength;
			remainingLength = 0;
		}
		else
		{
			// Fit the amount that will fit in the current chunk
			s32 lengthToCopy = remainingInChunk;
			buffer->lastChunk->used += lengthToCopy;
			buffer->byteCount += lengthToCopy;
			remainingLength -= lengthToCopy;

			// Get a new chunk
			appendNewChunk(buffer);
		}
	}

	return result;
}

void overwriteAt(WriteBuffer *buffer, s32 indexInBuffer, s32 length, void *data)
{
	// Find the chunk this starts in
	WriteBufferChunk *chunk;
	s32 chunkIndex = indexInBuffer / buffer->chunkSize;
	chunk = buffer->firstChunk;
	while (chunkIndex > 0)
	{
		chunk = chunk->nextChunk;
		chunkIndex--;
	}

	s32 posInChunk = indexInBuffer % buffer->chunkSize;

	s32 remainingLength = length;
	u8 *dataPos = (u8*) data;
	while (remainingLength > 0)
	{
		s32 remainingInChunk = buffer->chunkSize - posInChunk;
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
