#pragma once

struct BitmapFontGlyph
{
	unichar codepoint;
	Rect2I size;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character

	// TODO: Referring to a sprite too is redundant! Also, in some cases, we need to know the ^above data
	// but have progressed through the renderer so we only have the RenderItem. Maybe entirely use
	// BitmapFontGlyphs for text rendering instead?
	// u32 spriteID;
	struct Sprite *sprite;
};

struct BitmapFont
{
	String name;
	u32 spriteType;

	u16 lineHeight;
	u16 baseY;

	BitmapFontGlyph nullGlyph;

	u32 glyphCount;
	BitmapFontGlyph *glyphs;
};

struct BitmapFontCachedText
{
	V2 size;
	u32 charCount;
	struct RenderItem *chars;
};