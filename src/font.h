#pragma once

typedef u32 unichar;

struct BitmapFontChar
{
	unichar codepoint;
	Rect size;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character

	u32 textureRegionID;
};

struct BitmapFont
{
	TextureAssetType textureAssetType;

	u16 lineHeight;
	u16 baseY;

	BitmapFontChar nullChar;

	u32 charCount;
	BitmapFontChar *chars;
};

struct BitmapFontCachedText
{
	V2 size;
	u32 charCount;
	struct RenderItem *chars;
};