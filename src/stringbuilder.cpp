#pragma once

StringBuilder newStringBuilder(s32 initialSize, MemoryArena *arena)
{
	s32 allocationSize = max(initialSize, 256);

	StringBuilder b = {};
	b.arena = arena;
	b.buffer = allocateMultiple<char>(arena, allocationSize);
	b.currentMaxLength = allocationSize;
	b.length = 0;

	return b;
}

void expand(StringBuilder *stb, s32 newSize)
{
	DEBUG_FUNCTION();
	logWarn("Expanding StringBuilder"_s);
	
	s32 targetSize = max(newSize, stb->currentMaxLength * 2);

	char *newBuffer = allocateMultiple<char>(stb->arena, targetSize);
	copyMemory(stb->buffer, newBuffer, stb->length);

	stb->buffer = newBuffer;
	stb->currentMaxLength = targetSize;
}

void append(StringBuilder *stb, char *source, s32 length)
{
	if ((stb->length + length) > stb->currentMaxLength)
	{
		expand(stb, stb->length + length);
	}

	copyMemory(source, stb->buffer + stb->length, length);
	stb->length += length;
}

inline void append(StringBuilder *stringBuilder, String source)
{
	append(stringBuilder, source.chars, source.length);
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
