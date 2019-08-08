#pragma once

StringBuilder newStringBuilder(s32 initialSize, MemoryArena *arena)
{
	StringBuilder b = {};
	b.arena = arena;
	b.buffer = allocateMultiple<char>(arena, initialSize);
	b.currentMaxLength = initialSize;
	b.length = 0;

	return b;
}

void expand(StringBuilder *stb, s32 newSize)
{
	DEBUG_FUNCTION();
	
	s32 targetSize = newSize;

	if (targetSize == -1) 
	{
		targetSize = stb->currentMaxLength * 2;
	}
	else if (targetSize < stb->currentMaxLength)
	{
		DEBUG_BREAK();
		return;
	}

	char *newBuffer = allocateMultiple<char>(stb->arena, targetSize);
	copyMemory(stb->buffer, newBuffer, stb->currentMaxLength);

	stb->buffer = newBuffer;
	stb->currentMaxLength = targetSize;
}

void append(StringBuilder *stb, char *source, s32 length)
{
	s32 lengthToCopy = length;
	if ((stb->length + length) > stb->currentMaxLength)
	{
		s32 newMaxLength = max(stb->length + length, stb->currentMaxLength * 2);
		expand(stb, newMaxLength);
	}

	copyMemory(source, stb->buffer + stb->length, lengthToCopy);
	stb->length += lengthToCopy;
}

inline void append(StringBuilder *stringBuilder, String source)
{
	append(stringBuilder, source.chars, source.length);
}

inline void append(StringBuilder *stringBuilder, char *source)
{
	append(stringBuilder, makeString(source));
}

inline void append(StringBuilder *stringBuilder, char source)
{
	append(stringBuilder, &source, 1);
}

inline void append(StringBuilder *stringBuilder, StringBuilder *source)
{
	append(stringBuilder, source->buffer, source->length);
}

inline String getString(StringBuilder *stb)
{
	String result = makeString(stb->buffer, stb->length);
	return result;
}
