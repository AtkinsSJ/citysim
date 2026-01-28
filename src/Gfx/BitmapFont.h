/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Gfx/Forward.h>
#include <Util/Alignment.h>
#include <Util/Rectangle.h>
#include <Util/String.h>
#include <Util/Vector.h>

struct BitmapFontGlyph {
    unichar codepoint;
    u16 width;
    u16 height;
    s16 xOffset; // Offset when rendering to the screen
    s16 yOffset;
    s16 xAdvance; // How far to move after rendering this character

    Rect2 uv;
};

struct BitmapFontGlyphEntry {
    bool isOccupied;
    unichar codepoint;
    BitmapFontGlyph glyph;
};

class BitmapFont {
public:
    static BitmapFont& get(StringView name);

    static bool load_from_bmf_data(Blob data, AssetMetadata& asset);

    void add_glyph(BitmapFontGlyph&&);
    BitmapFontGlyph* find_glyph(unichar target_char) const;

    V2I calculate_text_size(StringView text, s32 max_width = 0) const;
    s32 calculate_max_text_width(std::initializer_list<StringView> texts, s32 limit = 0) const;

    u16 line_height() const { return m_line_height; }
    AssetMetadata* texture() const { return m_texture; }

private:
    BitmapFontGlyphEntry* find_glyph_entry(unichar target_char) const;

    // FIXME: Should be private, but we poke at them in loadBMFont() which is a free function
    u16 m_line_height {};
    u16 m_base_y {};

    // Hash-table style thing
    u32 m_glyph_count {};
    u32 m_glyph_capacity {};
    BitmapFontGlyphEntry* m_glyph_entries {};

    AssetMetadata* m_texture {};
};

struct DrawTextResult {
    bool isValid;
    V2I caretPosition;
};

// PUBLIC

V2I calculateTextPosition(V2I origin, V2I size, Alignment align);

// NB: If caretPosition is not -1, and caretInfoResult is non-null, caretInfoResult is filled with the data
// for the glyph at that position. (Intended use is so TextInput knows where its caret should appear.)
// Note that if there are no glyphs rendered (either because `text` is empty, or none of its characters
// were found in `font`) that no caretInfoResult data will be provided. You can check DrawTextResult.isValid
// to see if it has been filled in or not.
void drawText(RenderBuffer* renderBuffer, BitmapFont* font, StringView text, Rect2I bounds, Alignment align, Colour color, s8 shaderID, s32 caretIndex = -1, DrawTextResult* caretInfoResult = nullptr);
