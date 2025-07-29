/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBuilder.h"
#include "../debug.h"
#include <Util/Log.h>
#include <Util/Memory.h>

StringBuilder newStringBuilder(s32 initialSize, MemoryArena* arena)
{
    s32 allocationSize = max(initialSize, 256);

    StringBuilder b = {};
    b.arena = arena;
    b.buffer = arena->allocate_multiple<char>(allocationSize);
    b.currentMaxLength = allocationSize;
    b.length = 0;

    return b;
}

void expand(StringBuilder* stb, s32 newSize)
{
    DEBUG_FUNCTION();
    logWarn("Expanding StringBuilder"_s);

    s32 targetSize = max(newSize, stb->currentMaxLength * 2);

    char* newBuffer = stb->arena->allocate_multiple<char>(targetSize);
    copyMemory(stb->buffer, newBuffer, stb->length);

    stb->buffer = newBuffer;
    stb->currentMaxLength = targetSize;
}

void append(StringBuilder* stb, char* source, s32 length)
{
    if ((stb->length + length) > stb->currentMaxLength) {
        expand(stb, stb->length + length);
    }

    copyMemory(source, stb->buffer + stb->length, length);
    stb->length += length;
}

void append(StringBuilder* stringBuilder, String source)
{
    append(stringBuilder, source.chars, source.length);
}

void append(StringBuilder* stringBuilder, char source)
{
    append(stringBuilder, &source, 1);
}

void append(StringBuilder* stringBuilder, StringBuilder* source)
{
    append(stringBuilder, source->buffer, source->length);
}

String getString(StringBuilder* stb)
{
    String result = makeString(stb->buffer, stb->length);
    return result;
}
