#pragma once

struct BitmapFontGlyph
{
	unichar codepoint;
	s16 width, height;
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

struct DrawTextResult
{
	bool isValid;
	struct RenderItem *renderItemAtPosition;
	BitmapFontGlyph *glyphAtPosition;
};

// PUBLIC

BitmapFontGlyph *addGlyph(BitmapFont *font, unichar targetChar);
BitmapFontGlyph *findChar(BitmapFont *font, unichar targetChar);

V2 calculateTextSize(BitmapFont *font, String text, f32 maxWidth=0);
V2 calculateTextPosition(V2 origin, V2 size, u32 align);

// NB: If caretPosition is not -1, and caretInfoResult is non-null, caretInfoResult is filled with the data
// for the glyph at that position. (Intended use is so TextInput knows where its caret should appear.)
// Note that if there are no glyphs rendered (either because `text` is empty, or none of its characters
// were found in `font`) that no caretInfoResult data will be provided. You can check DrawTextResult.isValid
// to see if it has been filled in or not.
void drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2 topLeft, f32 maxWidth, f32 depth, V4 color, s32 shaderID, s32 caretPosition=-1, DrawTextResult *caretInfoResult=null);

// INTERNAL

BitmapFontGlyphEntry *findGlyphInternal(BitmapFont *font, unichar targetChar);
struct DrawTextState
{
	bool doWrap;
	f32 maxWidth;
	f32 lineHeight;
	s32 lineCount = 1;

	V2 origin;
	V2 currentPositionRelative = v2(0,0);

	RenderItem *firstRenderItem;
	s32 startOfCurrentWord = 0;
	s32 endOfCurrentWord = 0;
	f32 currentWordWidth = 0;

	f32 longestLineWidth = 0;

	DrawTextState(f32 maxWidth, f32 lineHeight, V2 origin=v2(0,0), RenderItem *firstRenderItem=null)
		: maxWidth(maxWidth), doWrap(maxWidth > 0), lineHeight(lineHeight), origin(origin), firstRenderItem(firstRenderItem)
	{}
};
void nextLine(DrawTextState *state);
void handleWrapping(DrawTextState *state, BitmapFontGlyph *c);
