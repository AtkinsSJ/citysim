#pragma once

StringBuilder newStringBuilder(s32 initialSize, MemoryArena *arena)
{
	StringBuilder b = {};
	b.arena = arena;
	b.buffer = PushArray(arena, char, initialSize);
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

	char *newBuffer = PushArray(stb->arena, char, targetSize);
	for (s32 i=0; i<stb->currentMaxLength; i++)
	{
		newBuffer[i] = stb->buffer[i];
	}

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

	for (s32 i=0; i < lengthToCopy; i++)
	{
		stb->buffer[stb->length++] = source[i];
	}
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
