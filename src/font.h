#pragma once

struct BitmapFontChar
{
	uint32 id; // Unicode character
	Rect size;
	int16 xOffset, yOffset; // Offset when rendering to the screen
	int16 xAdvance; // How far to move after rendering this character

	GLint textureID;
	RealRect uv;
};

struct BitmapFont
{
	uint16 lineHeight;
	uint16 baseY;

	BitmapFontChar nullChar;

	uint32 charCount;
	BitmapFontChar *chars;
};

struct BitmapFontCachedText
{
	V2 size;
	uint32 spriteCount;
	Sprite *sprites;
};

#include "font.cpp"