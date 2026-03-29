/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <Util/Basic.h>
#include <Util/String.h>

bool byte_is_start_of_glyph(char b);

// Decodes the byte to see how long it claims to be.
// There is no guarantee that the bytes that follow it are valid within the string, we don't check!
// returns 0 for an invalid start byte
size_t length_of_glyph(char start_byte);

// Determines how many utf8 bytes are required to store the given unichar
size_t length_of_unichar(unichar c);

// returns 0 (start of the buffer) if can't find the start of the glyph
size_t find_start_of_glyph(ReadonlySpan<char> const& buffer, size_t start_offset);

// returns -1 if no next glyph exists
s32 find_start_of_next_glyph(ReadonlySpan<char> const& buffer, size_t start_offset);

struct GlyphAndByteCounts {
    size_t glyph_count { 0 };
    size_t byte_count { 0 };
};
GlyphAndByteCounts floor_to_whole_glyphs(ReadonlySpan<char>);

// If the first char is not a start byte, we return 0.
unichar read_unicode_char(ReadonlySpan<char> buffer);

bool is_whitespace(unichar u_char, bool include_newlines = true);

bool is_newline(unichar u_char);

// Byte pos is read and written to.
bool getNextUnichar(StringView string, s32* bytePos, unichar* result);
