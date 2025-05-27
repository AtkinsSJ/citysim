#pragma once

struct BitmapFontGlyph {
    unichar codepoint;
    s16 width, height;
    s16 xOffset, yOffset; // Offset when rendering to the screen
    s16 xAdvance;         // How far to move after rendering this character

    Rect2 uv;
};

struct BitmapFontGlyphEntry {
    bool isOccupied;
    unichar codepoint;
    BitmapFontGlyph glyph;
};

struct BitmapFont {
    String name;

    u16 lineHeight;
    u16 baseY;

    // Hash-table style thing
    u32 glyphCount;
    u32 glyphCapacity;
    BitmapFontGlyphEntry* glyphEntries;

    struct Asset* texture;
};

struct DrawTextResult {
    bool isValid;
    V2I caretPosition;
};

// PUBLIC

BitmapFontGlyph* addGlyph(BitmapFont* font, unichar targetChar);
BitmapFontGlyph* findChar(BitmapFont* font, unichar targetChar);

V2I calculateTextSize(BitmapFont* font, String text, s32 maxWidth = 0);
V2I calculateTextPosition(V2I origin, V2I size, u32 align);
s32 calculateMaxTextWidth(BitmapFont* font, std::initializer_list<String> texts, s32 limit = 0);

// NB: If caretPosition is not -1, and caretInfoResult is non-null, caretInfoResult is filled with the data
// for the glyph at that position. (Intended use is so TextInput knows where its caret should appear.)
// Note that if there are no glyphs rendered (either because `text` is empty, or none of its characters
// were found in `font`) that no caretInfoResult data will be provided. You can check DrawTextResult.isValid
// to see if it has been filled in or not.
void drawText(struct RenderBuffer* renderBuffer, BitmapFont* font, String text, Rect2I bounds, u32 align, V4 color, s8 shaderID, s32 caretIndex = -1, DrawTextResult* caretInfoResult = nullptr);

// INTERNAL

BitmapFontGlyphEntry* findGlyphInternal(BitmapFont* font, unichar targetChar);
