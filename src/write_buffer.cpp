#pragma once

void initWriteBuffer(WriteBuffer *buffer, s32 chunkSize, MemoryArena *arena)
{
	buffer->arena = arena;
	buffer->chunkSize = chunkSize;
	buffer->byteCount = 0;

	buffer->chunkCount = 1;
	buffer->firstChunk = buffer->lastChunk = allocateWriteBufferChunk(buffer);
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
			WriteBufferChunk *newChunk = allocateWriteBufferChunk(buffer);
			buffer->lastChunk->nextChunk = newChunk;
			buffer->lastChunk = newChunk;
			buffer->chunkCount++;
		}
	}
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
			WriteBufferChunk *newChunk = allocateWriteBufferChunk(buffer);
			buffer->lastChunk->nextChunk = newChunk;
			buffer->lastChunk = newChunk;
			buffer->chunkCount++;
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
			copyMemory(dataPos, (buffer->lastChunk->bytes + posInChunk), remainingLength);
			remainingLength = 0;
		}
		else
		{
			// Copy the amount that will fit in the current chunk
			s32 lengthToCopy = remainingInChunk;
			copyMemory(dataPos, (buffer->lastChunk->bytes + posInChunk), lengthToCopy);
			dataPos += lengthToCopy;
			remainingLength -= lengthToCopy;

			// Go to next chunk
			chunk = chunk->nextChunk;
			posInChunk = 0;
		}
	}
}

WriteBufferChunk *allocateWriteBufferChunk(WriteBuffer *buffer)
{
	Blob blob = allocateBlob(buffer->arena, sizeof(WriteBufferChunk) + buffer->chunkSize);
	WriteBufferChunk *result = (WriteBufferChunk *)blob.memory;
	result->used = 0;
	result->bytes = (u8 *) (blob.memory + sizeof(WriteBufferChunk));
	result->nextChunk = null;

	return result;
}
