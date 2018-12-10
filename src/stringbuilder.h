#pragma once

/**
 * This is for temporary string allocation, like in myprintf().
 * As such, when it runs out of room it allocates a new buffer without deallocating the old one - 
 * because we're assuming we'll use a temporary memory arena that will be reset once we're done.
 * So, BE CAREFUL!
 * If you want to keep the resulting String around after, make a copy of it somewhere permanent.
 *
 * It's permitted to pass any MemoryArena you like, and it will use that for allocations. So, if
 * you want to keep it around long term, you can use a separate arena and reset that once you're
 * done.
 */

struct StringBuilder
{
	MemoryArena *arena;
	char *buffer;
	s32 length;
	s32 currentMaxLength;
};

StringBuilder newStringBuilder(s32 initialSize, MemoryArena *arena=globalFrameTempArena)
{
	StringBuilder b = {};
	b.arena = arena;
	b.buffer = PushArray(arena, char, initialSize);
	b.currentMaxLength = initialSize;
	b.length = 0;

	return b;
}

// NB: As mentioned above, the old buffer is NOT deallocated!
void expand(StringBuilder *stb, s32 newSize=-1)
{
	s32 targetSize = newSize;
	if (targetSize == -1) targetSize = stb->currentMaxLength * 2;

	ASSERT(targetSize > stb->currentMaxLength, "OOPS");

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
		s32 newMaxLength = MAX(stb->length + length, stb->currentMaxLength * 2);
		expand(stb, newMaxLength);
	}

	for (s32 i=0; i < lengthToCopy; i++)
	{
		stb->buffer[stb->length++] = source[i];
	}
}

void append(StringBuilder *stringBuilder, String source)
{
	append(stringBuilder, source.chars, source.length);
}

void append(StringBuilder *stringBuilder, char *source)
{
	append(stringBuilder, source, strlen(source));
}

void append(StringBuilder *stringBuilder, char source)
{
	append(stringBuilder, &source, 1);
}

void append(StringBuilder *stringBuilder, StringBuilder *source)
{
	append(stringBuilder, source->buffer, source->length);
}

String getString(StringBuilder *stb)
{
	String result = {};
	result.chars = stb->buffer;
	result.length = stb->length;

	return result;
}