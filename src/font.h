#pragma once

struct BitmapFontChar
{
	unichar codepoint;
	Rect size;
	int16 xOffset, yOffset; // Offset when rendering to the screen
	int16 xAdvance; // How far to move after rendering this character

	uint32 textureRegionID;
};

struct BitmapFont
{
	TextureAssetType textureAssetType;

	uint16 lineHeight;
	uint16 baseY;

	BitmapFontChar nullChar;

	uint32 charCount;
	BitmapFontChar *chars;
};

struct BitmapFontCachedText
{
	V2 size;
	uint32 charCount;
	struct RenderItem *chars;
};