#pragma once

struct StringBuffer
{
	char *buffer;
	int32 bufferLength;
	int32 bufferMaxLength;
};

StringBuffer newStringBuffer(MemoryArena *arena, int32 length)
{
	StringBuffer b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.bufferMaxLength = length;

	return b;
}

void appendToBuffer(StringBuffer *buffer, char *source, int32 length)
{
	int32 lengthToCopy = length;
	if ((buffer->bufferLength + length) > buffer->bufferMaxLength)
	{
		length = buffer->bufferMaxLength - buffer->bufferLength;
	}

	for (int32 i=0; i < lengthToCopy; i++)
	{
		buffer->buffer[buffer->bufferLength++] = source[i];
	}
}