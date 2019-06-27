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

StringBuilder newStringBuilder(s32 initialSize, MemoryArena *arena=globalFrameTempArena);

// NB: As mentioned above, the old buffer is NOT deallocated!
void expand(StringBuilder *stb, s32 newSize=-1);

void append(StringBuilder *stb, char *source, s32 length);
void append(StringBuilder *stringBuilder, String source);
void append(StringBuilder *stringBuilder, char *source);
void append(StringBuilder *stringBuilder, char source);
void append(StringBuilder *stringBuilder, StringBuilder *source);

String getString(StringBuilder *stb);
