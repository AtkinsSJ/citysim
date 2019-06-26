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

// PUBLIC

BitmapFontGlyph *addGlyph(BitmapFont *font, unichar targetChar);
BitmapFontGlyph *findChar(BitmapFont *font, unichar targetChar);

V2 calculateTextSize(BitmapFont *font, String text, f32 maxWidth=0);
V2 calculateTextPosition(V2 origin, V2 size, u32 align);
BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, String text, f32 maxWidth=0);
void drawCachedText(RenderBuffer *uiBuffer, BitmapFontCachedText *cache, V2 topLeft, f32 depth, V4 color, s32 shaderID);

// INTERNAL

BitmapFontGlyphEntry *findGlyphInternal(BitmapFont *font, unichar targetChar);
struct DrawTextState
{
	bool doWrap;
	f32 maxWidth;
	f32 lineHeight;
	s32 lineCount;

	V2 origin;
	V2 position;

	RenderItem *firstRenderItem;
	s32 startOfCurrentWord;
	s32 endOfCurrentWord;
	f32 currentWordWidth;

	f32 currentLineWidth;
	f32 longestLineWidth;
};
DrawTextState makeDrawTextState(f32 maxWidth, f32 lineHeight, RenderItem *firstRenderItem=null);
void nextLine(DrawTextState *state);
void handleWrapping(DrawTextState *state, BitmapFontGlyph *c);
