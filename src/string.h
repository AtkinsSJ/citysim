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

void append(StringBuffer *buffer, char *source, int32 length)
{
	int32 lengthToCopy = length;
	if ((buffer->bufferLength + length) > buffer->bufferMaxLength)
	{
		lengthToCopy = buffer->bufferMaxLength - buffer->bufferLength;
	}

	for (int32 i=0; i < lengthToCopy; i++)
	{
		buffer->buffer[buffer->bufferLength++] = source[i];
	}

	// Final null, just in case.
	buffer->buffer[buffer->bufferLength] = 0;
}

void append(StringBuffer *buffer, char *source)
{
	append(buffer, source, strlen(source));
}

void append(StringBuffer *dest, StringBuffer *src)
{
	append(dest, src->buffer, src->bufferLength);
}

void backspace(StringBuffer *buffer)
{
	if (buffer->bufferLength > 0)
	{
		buffer->buffer[--buffer->bufferLength] = 0;
	}
}

void clear(StringBuffer *buffer)
{
	buffer->bufferLength = 0;
	buffer->buffer[0] = 0;
}

bool equals(StringBuffer *buffer, char *other)
{
	bool result = true;
	int32 otherLength = strlen(other);
	if (otherLength != buffer->bufferLength)
	{
		result = false;
	}
	else
	{
		for (int32 i = 0; i<otherLength; i++)
		{
			if (buffer->buffer[i] != other[i])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}