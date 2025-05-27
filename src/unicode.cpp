#pragma once
#pragma once

inline bool byteIsStartOfGlyph(char b)
{
    // continuation bytes always start with 10xxxxxx
    // So if it doesn't, it must be the start byte!
    bool result = ((b & 0b11000000) != 0b10000000);

    return result;
}

// Decodes the byte to see how long it claims to be.
// There is no guarantee that the bytes that follow it are valid within the string, we don't check!
// returns 0 for an invalid start byte
inline s32 lengthOfGlyph(char startByte)
{
    s32 result = 0;

    if ((startByte & 0b10000000) == 0) {
        result = 1;
    } else if ((startByte & 0b11000000) == 0b11000000) {
        result = 2;

        if (startByte & 0b00100000) {
            result = 3;

            if (startByte & 0b00010000) {
                result = 4;
                if (startByte & 0b00001000) {
                    result = 5;

                    if (startByte & 0b00000100) {
                        result = 6;
                    }
                }
            }
        }
    }

    return result;
}

inline s32 lengthOfUnichar(unichar c)
{
    if (c <= 0x7F) {
        return 1;
    } else if (c <= 0x7FF) {
        return 2;
    } else if (c <= 0xFFFF) {
        return 3;
    } else {
        return 4;
    }
}

// returns 0 (start of the buffer) if can't find the start of the glyph
s32 findStartOfGlyph(char* buffer, s32 byteOffset)
{
    s32 pos = 0;

    if (buffer != nullptr) {
        pos = byteOffset;
        while ((pos > 0) && !byteIsStartOfGlyph(buffer[pos])) {
            pos--;
        }
    }

    return pos;
}

// returns -1 if no next glyph exists
s32 findStartOfNextGlyph(char* buffer, s32 byteOffset, s32 bufferByteLength)
{
    s32 result = -1;

    if (bufferByteLength > 0) {
        s32 pos = byteOffset + 1;

        while ((pos < bufferByteLength) && !byteIsStartOfGlyph(buffer[pos])) {
            pos++;
        }

        if (byteIsStartOfGlyph(buffer[pos])) {
            result = pos;
        }
    }

    return result;
}

// returns 0 if we start mid-way through a glyph
s32 floorToWholeGlyphs(char* startByte, s32 byteLength)
{
    // @Speed: Should be able to start at the end, find the start of that glyph, then see how many bytes it
    // is - that's all we need to know to floor it!

    s32 flooredByteCount = 0;

    // Only count if we start at the beginning of a glyph.
    // Otherwise, we return 0.
    if (byteLength > 0 && byteIsStartOfGlyph(*startByte)) {
        s32 pos = 0;
        s32 glyphLength = lengthOfGlyph(startByte[pos]);
        while (pos + glyphLength <= byteLength) {
            pos += glyphLength;
            flooredByteCount += glyphLength;
            glyphLength = lengthOfGlyph(startByte[pos]);
        }
    }

    return flooredByteCount;
}

// Counts how many full glyphs are in the buffer
s32 countGlyphs(char* startByte, s32 byteLength)
{
    s32 glyphCount = 0;

    if (byteLength > 0) {
        s32 pos = 0;

        while (pos < byteLength) {
            if (byteIsStartOfGlyph(startByte[pos])) {
                s32 glyphLength = lengthOfGlyph(startByte[pos]);

                if (glyphLength == 0) {
                    logError("Invalid unicode codepoint {2} at pos {0} of string '{1}'"_s, { formatInt(pos), makeString(startByte, byteLength), formatInt(startByte[pos], 16) });
                    break;
                } else {
                    glyphCount++;
                    pos += glyphLength;
                }
            }
        }
    }

    return glyphCount;
}

// If the first char is not a start byte, we return 0.
unichar readUnicodeChar(char* firstChar)
{
    unichar result = 0;

    if (firstChar != nullptr && byteIsStartOfGlyph(*firstChar)) {
        u8 b1 = *firstChar;
        if ((b1 & 0b10000000) == 0) {
            // 7-bit ASCII, so just pass through
            result = (u32)b1;
        }
        // NB: I disabled the extra checks for valid utf8, we're just going to assume it's valid because
        // that's faster and this gets called a lot. If we really care about possibly-malformed utf8, we
        // can just check any user or file input as it comes in.
        // - Sam, 23/06/2019
        else // if ((b1 & 0b11000000) == 0b11000000)
        {
            // Start of a multibyte codepoint!
            s32 extraBytes = 1;
            result = b1 & 0b00011111;

            if (b1 & 0b00100000) {
                extraBytes++; // 3 total
                result &= 0b00001111;

                if (b1 & 0b00010000) {
                    extraBytes++; // 4 total
                    result &= 0b00000111;

                    if (b1 & 0b00001000) {
                        extraBytes++; // 5 total
                        result &= 0b00000011;

                        if (b1 & 0b00000100) {
                            extraBytes++; // 6 total
                            result &= 0b00000001;
                        }
                    }
                }
            }

            for (s32 pos = 1; pos <= extraBytes; pos++) {
                u8 bn = firstChar[pos];
                // ASSERT((bn & 0b11000000) == 0b10000000); //Unicode codepoint continuation byte is invalid! D:
                result = (result << 6) | (bn & 0b00111111);
            }
        }
    }

    return result;
}

inline bool isWhitespace(unichar uChar, bool countNewlines)
{
    // NB: See note in isNewline() about why we use a switch. (Basically, it's faster!)

    bool result = false;

    switch (uChar) {
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
        result = true;
        break;

    // NB: Copied from isNewline(), so we can have this as a single switch
    case '\n':
    case '\r':
    case '\v':      // Vertical tab
    case '\f':      // Form feed
    case L'\u0085': // Unicode NEL (next line)
    case L'\u2028': // Unicode LS (line separator)
    case L'\u2029': // Unicode PS (paragraph separator)
        result = countNewlines;
        break;
    }

    return result;
}

inline bool isNewline(unichar uChar)
{
    bool result = false;

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
    switch (uChar) {
    case '\n':
    case '\r':
    case '\v':      // Vertical tab
    case '\f':      // Form feed
    case L'\u0085': // Unicode NEL (next line)
    case L'\u2028': // Unicode LS (line separator)
    case L'\u2029': // Unicode PS (paragraph separator)
        result = true;
        break;
    }

    return result;
}

inline bool getNextUnichar(String string, s32* bytePos, unichar* result)
{
    bool foundResult = false;

    if (*bytePos < string.length) {
        unichar c = readUnicodeChar(string.chars + *bytePos);

        *result = c;

        *bytePos += lengthOfUnichar(c);

        foundResult = true;
    }

    return foundResult;
}
