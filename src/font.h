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

	uint32 charCount;
	BitmapFontChar *chars;
};

struct BitmapFontCachedText
{
	V2 size;
	uint32 spriteCount;
	Sprite *sprites;
};

void drawText(GLRenderer *renderer, BitmapFont *font, V2 position, char *text, V4 *color=0);

BitmapFontCachedText *drawTextToCache(BitmapFont *font, char *text, V4 *color=0);

void drawCachedText(GLRenderer *renderer, BitmapFontCachedText *cache, V2 topLeft, real32 depth);

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, uint32 align);

#include "font.cpp"