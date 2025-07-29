/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BinaryFile.h"

void rleDecode(u8* source, u8* dest, smm destSize)
{
    u8* sourcePos = source;
    u8* destPos = dest;
    u8* destEnd = dest + destSize;

    while (destPos < destEnd) {
        s8 length = *((s8*)sourcePos);
        sourcePos++;
        if (length < 0) {
            // Literals
            s8 literalCount = -length;
            copyMemory(sourcePos, destPos, literalCount);
            sourcePos += literalCount;
            destPos += literalCount;
        } else {
            // RLE
            u8 value = *sourcePos;
            sourcePos++;
            fillMemory(destPos, value, length);
            destPos += length;
        }
    }
}
