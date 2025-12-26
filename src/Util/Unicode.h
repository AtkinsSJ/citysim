/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <Util/Basic.h>
#include <Util/String.h>

bool byteIsStartOfGlyph(char b);

// Decodes the byte to see how long it claims to be.
// There is no guarantee that the bytes that follow it are valid within the string, we don't check!
// returns 0 for an invalid start byte
s32 lengthOfGlyph(char startByte);

// Determines how many utf8 bytes are required to store the given unichar
s32 lengthOfUnichar(unichar c);

// returns 0 (start of the buffer) if can't find the start of the glyph
s32 findStartOfGlyph(char const* buffer, s32 byteOffset);

// returns -1 if no next glyph exists
s32 findStartOfNextGlyph(char const* buffer, s32 byteOffset, s32 bufferByteLength);

// returns 0 if we start mid-way through a glyph
s32 floorToWholeGlyphs(char const* startByte, s32 byteLength);

// Counts how many full glyphs are in the buffer
s32 countGlyphs(char const* startByte, s32 byteLength);

// If the first char is not a start byte, we return 0.
unichar readUnicodeChar(char const* firstChar);

bool isWhitespace(unichar uChar, bool countNewlines = true);

bool isNewline(unichar uChar);

// Byte pos is read and written to.
bool getNextUnichar(String string, s32* bytePos, unichar* result);
