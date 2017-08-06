#pragma once

bool byteIsStartOfGlyph(char b)
{
	// continuation bytes always start with 10xxxxxx
	// So if it doesn't, it must be the start byte!
	bool result = ((b & 0b11000000) != 0b10000000);

	return result;
}

// Decodes the byte to see how long it claims to be.
// There is no guarantee that the bytes that follow it are valid within the string, we don't check!
// returns 0 for an invalid start byte
int32 lengthOfGlyph(char startByte)
{
	int32 result = 0;

	if ((startByte & 0b10000000) == 0)
	{
		result = 1;
	}
	else if (startByte & 0b11000000)
	{
		result = 2;

		if (startByte & 0b00100000)
		{
			result = 3;

			if (startByte & 0b00010000)
			{
				result = 4;
				if (startByte & 0b00001000)
				{
					result = 5;

					if (startByte & 0b00000100)
					{
						result = 6;
					}
				}
			}
		}
	}

	return result;
}

bool isFullGlyph(char *buffer, int32 glyphStartPos, int32 bufferByteLength)
{
	bool result = false;

	if (byteIsStartOfGlyph(buffer[glyphStartPos]))
	{
		int32 glyphLength = lengthOfGlyph(buffer[glyphStartPos]);

		if (glyphLength == 0)
		{
			result = false;
		}
		else
		{
			result = (glyphStartPos + glyphLength - 1) < bufferByteLength;
		}
	}

	return result;
}

// returns 0 (start of the buffer) if can't find the start of the glyph
int32 findStartOfGlyph(char *buffer, int32 byteOffset)
{
	int32 pos = byteOffset;
	while ((pos > 0) && !byteIsStartOfGlyph(buffer[pos]) )
	{
		pos--;
	}

	return pos;
}

// returns -1 if no next glyph exists
int32 findStartOfNextGlyph(char *buffer, int32 byteOffset, int32 bufferByteLength)
{
	int32 pos = byteOffset + 1;

	while ((pos < bufferByteLength) && !byteIsStartOfGlyph(buffer[pos]))
	{
		pos++;
	}

	int32 result = -1;

	if (byteIsStartOfGlyph(buffer[pos]))
	{
		result = pos;
	}

	return result;
}

// returns 0 if we start mid-way through a glyph
int32 floorToWholeGlyphs(char *startByte, int32 byteLength)
{
	int32 flooredByteCount = 0;

	// Only count if we start at the beginning of a glyph.
	// Otherwise, we return 0.
	if (byteIsStartOfGlyph(*startByte))
	{
		int32 pos = 0;
		int32 glyphLength = lengthOfGlyph(startByte[pos]);
		while (pos + glyphLength <= byteLength)
		{
			pos += glyphLength;
			flooredByteCount += glyphLength;
			glyphLength = lengthOfGlyph(startByte[pos]);
		}
	}

	return flooredByteCount;
}

// Counts how many full glyphs are in the buffer
int32 countGlyphs(char *startByte, int32 byteLength)
{
	int32 glyphCount = 0;

	// Check that the byte we start on is actually a glyph start byte!
	// Otherwise our result will be meaningless.
	ASSERT(byteIsStartOfGlyph(*startByte), "Can't count glyphs starting part-way through a glyph!");

	int32 pos = 0;
	while (pos != -1)
	{
		if (!isFullGlyph(startByte, pos, byteLength))
		{
			break;
		}

		glyphCount++;
		pos = findStartOfNextGlyph(startByte, pos, byteLength);
	}

	return glyphCount;
}

// If the first char is not a start byte, we return 0.
unichar readUnicodeChar(char *firstChar)
{
	unichar result = 0;

	if (byteIsStartOfGlyph(*firstChar))
	{
		uint8 b1 = *firstChar;
		if ((b1 & 0b10000000) == 0)
		{
			// 7-bit ASCII, so just pass through
			result = (uint32) b1;
		}
		else if (b1 & 0b11000000)
		{
			// Start of a multibyte codepoint!
			int32 extraBytes = 1;
			result = b1 & 0b00011111;
			if (b1 & 0b00100000) {
				extraBytes++; // 3 total
				result = b1 & 0b00001111;
				if (b1 & 0b00010000) {
					extraBytes++; // 4 total
					result = b1 & 0b00000111;
					if (b1 & 0b00001000) {
						extraBytes++; // 5 total
						result = b1 & 0b00000011;
						if (b1 & 0b00000100) {
							extraBytes++; // 6 total
							result = b1 & 0b00000001;
						}
					}
				}
			}

			for (int32 pos = 0; pos < extraBytes; pos++)
			{
				result = result << 6;

				uint8 bn = firstChar[pos+1];

				if (!(bn & 0b10000000)
					|| (bn & 0b01000000))
				{
					ASSERT(false, "Unicode codepoint continuation byte is invalid! D:");
				}

				result |= (bn & 0b00111111);
			}
		}
	}
	
	return result;
}