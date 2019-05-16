#pragma once

struct BitmapFontGlyph
{
	unichar codepoint;
	Rect2I size;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character

	u32 page;
	Rect2 uv;

	// TODO: Referring to a sprite too is redundant! Also, in some cases, we need to know the ^above data
	// but have progressed through the renderer so we only have the RenderItem. Maybe entirely use
	// BitmapFontGlyphs for text rendering instead?
	// u32 spriteID;
	struct Sprite *sprite;
};

struct BitmapFontGlyphEntry
{
	bool isOccupied;
	unichar codepoint;
	BitmapFontGlyph glyph;
};

const s32 fontGlyphCapacityMultiplier = 2;

struct BitmapFont
{
	String name;
	struct Asset *spriteGroup; // TODO: Remove this, instead just put the relevant data inside the BitmapFontGlyph

	u16 lineHeight;
	u16 baseY;

	BitmapFontGlyph nullGlyph;

	// Hash-table style thing
	u32 glyphCount;
	u32 glyphCapacity;
	BitmapFontGlyphEntry *glyphEntries;

	u32 pageCount;
	Asset **pageTextures;
};

struct BitmapFontCachedText
{
	V2 size;
	u32 charCount;
	struct RenderItem *chars;
};