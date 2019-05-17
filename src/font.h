#pragma once

struct BitmapFontGlyph
{
	unichar codepoint;
	Rect2I size;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character

	u32 page;
	Rect2 uv;
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

	u16 lineHeight;
	u16 baseY;

	BitmapFontGlyph nullGlyph;

	// Hash-table style thing
	u32 glyphCount;
	u32 glyphCapacity;
	BitmapFontGlyphEntry *glyphEntries;

	u32 pageCount;
	struct Asset **pageTextures;
};

struct BitmapFontCachedText
{
	V2 bounds;
	
	// These are parallel arrays. renderItems for the render items, glyphs for the font glyph info
	u32 glyphCount;
	struct RenderItem *renderItems;
	struct BitmapFontGlyph **glyphs; 
};