/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Unicode.h"
#include <Util/Log.h>
#include <Util/Span.h>

bool byte_is_start_of_glyph(char b)
{
    // Continuation bytes always start with 10xxxxxx
    // So if it doesn't, it must be the start byte!
    return (b & 0b11000000) != 0b10000000;
}

// Decodes the byte to see how long it claims to be.
// There is no guarantee that the bytes that follow it are valid within the string, we don't check!
// returns 0 for an invalid start byte
size_t length_of_glyph(char start_byte)
{
    s32 result = 0;

    if ((start_byte & 0b10000000) == 0) {
        result = 1;
    } else if ((start_byte & 0b11000000) == 0b11000000) {
        result = 2;

        if (start_byte & 0b00100000) {
            result = 3;

            if (start_byte & 0b00010000) {
                result = 4;
                if (start_byte & 0b00001000) {
                    result = 5;

                    if (start_byte & 0b00000100) {
                        result = 6;
                    }
                }
            }
        }
    }

    return result;
}

size_t length_of_unichar(unichar c)
{
    if (c <= 0x7F)
        return 1;
    if (c <= 0x7FF)
        return 2;
    if (c <= 0xFFFF)
        return 3;
    return 4;
}

// returns 0 (start of the buffer) if can't find the start of the glyph
size_t find_start_of_glyph(ReadonlySpan<char> const& buffer, size_t start_offset)
{
    auto pos = start_offset;
    while ((pos > 0) && !byte_is_start_of_glyph(buffer[pos])) {
        pos--;
    }
    return pos;
}

// returns -1 if no next glyph exists
// FIXME: Return Optional<> instead
s32 find_start_of_next_glyph(ReadonlySpan<char> const& buffer, size_t start_offset)
{
    s32 result = -1;

    if (buffer.size() > 0) {
        auto pos = start_offset + 1;

        while ((pos < buffer.size()) && !byte_is_start_of_glyph(buffer[pos])) {
            pos++;
        }

        if (byte_is_start_of_glyph(buffer[pos])) {
            result = pos;
        }
    }

    return result;
}

GlyphAndByteCounts floor_to_whole_glyphs(ReadonlySpan<char> buffer)
{
    GlyphAndByteCounts counts;

    if (buffer.size() > 0) {
        size_t pos = 0;

        while (pos < buffer.size()) {
            if (byte_is_start_of_glyph(buffer[pos])) {
                auto glyph_length = length_of_glyph(buffer[pos]);

                if (glyph_length == 0) {
                    logError("Invalid unicode codepoint {2} at pos {0} of string '{1}'"_s, { formatInt(pos), String { buffer.raw_data(), buffer.size() }, formatInt(buffer[pos], 16) });
                    break;
                }
                counts.glyph_count++;
                counts.byte_count += glyph_length;
                pos += glyph_length;
            }
        }
    }

    return counts;
}

// If the first char is not a start byte, we return 0.
unichar read_unicode_char(ReadonlySpan<char> buffer)
{
    unichar result = 0;

    if (!buffer.is_empty() && byte_is_start_of_glyph(buffer[0])) {
        auto b1 = buffer[0];
        if ((b1 & 0b10000000) == 0) {
            // 7-bit ASCII, so just pass through
            result = static_cast<unichar>(b1);
        }
        // NB: I disabled the extra checks for valid utf8, we're just going to assume it's valid because
        // that's faster and this gets called a lot. If we really care about possibly-malformed utf8, we
        // can just check any user or file input as it comes in.
        // - Sam, 23/06/2019
        else // if ((b1 & 0b11000000) == 0b11000000)
        {
            // Start of a multibyte codepoint!
            size_t extra_bytes = 1;
            result = b1 & 0b00011111;

            if (b1 & 0b00100000) {
                extra_bytes++; // 3 total
                result &= 0b00001111;

                if (b1 & 0b00010000) {
                    extra_bytes++; // 4 total
                    result &= 0b00000111;

                    if (b1 & 0b00001000) {
                        extra_bytes++; // 5 total
                        result &= 0b00000011;

                        if (b1 & 0b00000100) {
                            extra_bytes++; // 6 total
                            result &= 0b00000001;
                        }
                    }
                }
            }

            for (auto pos = 1u; pos <= extra_bytes; pos++) {
                auto bn = buffer[pos];
                // ASSERT((bn & 0b11000000) == 0b10000000); //Unicode codepoint continuation byte is invalid! D:
                result = (result << 6) | (bn & 0b00111111);
            }
        }
    }

    return result;
}

bool is_whitespace(unichar u_char, bool include_newlines)
{
    // NB: See note in isNewline() about why we use a switch. (Basically, it's faster!)
    switch (u_char) {
    // TODO: @Cleanup We handle null as a whitespace, but it's not actually one.
    // Keeping this here for now to avoid breaking stuff, but should check and remove if
    // it's not useful, and correct places we do need it.
    case 0:
    case ' ':
    case '\t':
    case L'\u00A0': // Non-breaking space
    case L'\u1680': // Ogham space mark
    case L'\u2000': // En quad
    case L'\u2001': // Em quad
    case L'\u2002': // En space
    case L'\u2003': // Em space
    case L'\u2004': // Three-per-em space
    case L'\u2005': // Four-per-em space
    case L'\u2006': // Six-per-em space
    case L'\u2007': // Figure space
    case L'\u2008': // Punctuation space
    case L'\u2009': // Thin space
    case L'\u200A': // Hair space
    case L'\u202F': // Narrow no-break space
    case L'\u205F': // Medium mathematical space
    case L'\u3000': // Ideographic space
        return true;

    // NB: Copied from isNewline(), so we can have this as a single switch
    case '\n':
    case '\r':
    case '\v':      // Vertical tab
    case '\f':      // Form feed
    case L'\u0085': // Unicode NEL (next line)
    case L'\u2028': // Unicode LS (line separator)
    case L'\u2029': // Unicode PS (paragraph separator)
        return include_newlines;

    default:
        return false;
    }
}

bool is_newline(unichar u_char)
{
    //
    // In testing, the switch below was significantly faster than doing a chain of comparissons like:
    //
    // bool result = (uChar == '\n')
    //            || (uChar == '\r')
    //            || (uChar == '\v')      // Vertical tab
    //            || (uChar == '\f')      // Form feed
    //            || (uChar == L'\u0085')  // Unicode NEL (next line)
    //            || (uChar == L'\u2028')  // Unicode LS (line separator)
    //            || (uChar == L'\u2029'); // Unicode PS (paragraph separator)
    //
    // (Text rendering was about 5% faster this way. (3.0ms to 2.9ms) That's not a direct measurement of
    // isNewLine(), but for such a small function, it obviously has a big effect!)
    //
    // - Sam, 10/07/2019
    //
    switch (u_char) {
    case '\n':
    case '\r':
    case '\v':      // Vertical tab
    case '\f':      // Form feed
    case L'\u0085': // Unicode NEL (next line)
    case L'\u2028': // Unicode LS (line separator)
    case L'\u2029': // Unicode PS (paragraph separator)
        return true;

    default:
        return false;
    }
}

// FIXME: Replace with an iterator of some kind
bool getNextUnichar(StringView string, s32* bytePos, unichar* result)
{
    bool foundResult = false;

    if (*bytePos < string.length()) {
        unichar c = read_unicode_char(string.bytes().slice(*bytePos));

        *result = c;

        *bytePos += length_of_unichar(c);

        foundResult = true;
    }

    return foundResult;
}
