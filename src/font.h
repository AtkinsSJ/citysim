#pragma once

struct BitmapFontChar
{
	uint32 id; // Unicode character
	Rect size;
	int16 xOffset, yOffset; // Offset when rendering to the screen
	int16 xAdvance; // How far to move after rendering this character

	GLuint textureID;
	RealRect uv;
};

struct BitmapFont
{
	uint16 lineHeight;
	uint16 baseY;

	uint32 charCount;
	BitmapFontChar *chars;
};

void drawText(GLRenderer *renderer, BitmapFont *font, V2 position, char *text);

#include "font.cpp"