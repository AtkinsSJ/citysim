#pragma once

bool byteIsStartOfGlyph(char b)
{
	bool result = false;

	// continuation bytes always start with 10xxxxxx
	// So if it doesn't, it must be the start byte!
	if ((b & 0b11000000) == 0b10000000)
	{
		result = false;
	}
	else
	{
		result = true;
	}

	return result;
}

int32 floorToWholeGlyphs(char *startByte, int32 byteLength)
{

}

int32 countGlyphs(char *startByte, int32 byteLength)
{

}

int32 findStartOfGlyph(char *buffer, int32 byteOffset)
{

}

int32 findStartOfNextGlyph(char *buffer, int32 byteOffset, int32 bufferByteLength)
{

}