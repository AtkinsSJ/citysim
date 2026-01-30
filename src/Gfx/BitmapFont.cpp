/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BitmapFont.h"
#include <Assets/AssetManager.h>
#include <Debug/Debug.h>
#include <Gfx/Renderer.h>
#include <Util/Log.h>
#include <Util/Unicode.h>

// See: http://www.angelcode.com/products/bmfont/doc/file_format.html

#pragma pack(push, 1)
struct BMFontHeader {
    u8 tag[3];
    u8 version;
};
u8 const BMFontTag[3] = { 'B', 'M', 'F' };
u8 const BMFontSupportedVersion = 3;

struct BMFontBlockHeader {
    u8 type;
    u32 size;
};
enum BMFontBlockTypes {
    BMF_Block_Info = 1,
    BMF_Block_Common = 2,
    BMF_Block_Pages = 3,
    BMF_Block_Chars = 4,
    BMF_Block_KerningPairs = 5,
};

enum BMFont_ColorChannelType {
    BMFont_CCT_Glyph = 0,
    BMFont_CCT_Outline = 1,
    BMFont_CCT_Combined = 2, // Glyph and outline
    BMFont_CCT_Zero = 3,
    BMFont_CCT_One = 4,
};
struct BMFontBlock_Common {
    u16 lineHeight;
    u16 base;           // Distance from top of line to the character baseline
    u16 scaleW, scaleH; // GL_Texture dimensions
    u16 pageCount;      // How many texture pages?

    u8 bitfield; // 1 if 8-bit character data is packed into all channels
    u8 alphaChannel;
    u8 redChannel;
    u8 greenChannel;
    u8 blueChannel;
};

struct BMFont_Char {
    u32 id;
    u16 x, y;
    u16 w, h;
    s16 xOffset, yOffset; // Offset when rendering to the screen
    s16 xAdvance;         // How far to move after rendering this character
    u8 page;              // GL_Texture page
    u8 channel;           // Bitfield, 1 = blue, 2 = green, 4 = red, 8 = alpha
};

#pragma pack(pop)

BitmapFont& BitmapFont::get(StringView name)
{
    return getAsset(AssetType::BitmapFont, name.deprecated_to_string()).bitmapFont;
}

bool BitmapFont::load_from_bmf_data(Blob data, AssetMetadata& asset)
{
    smm pos = 0;
    BMFontHeader* header = (BMFontHeader*)(data.data() + pos);
    pos += sizeof(BMFontHeader);

    // Check it's a valid BMF
    if (header->tag[0] != BMFontTag[0]
        || header->tag[1] != BMFontTag[1]
        || header->tag[2] != BMFontTag[2]) {
        logError("Not a valid BMFont file: {0}"_s, { asset.fullName });
        return false;
    }

    if (header->version != 3) {
        logError("BMFont file version is unsupported: {0}, wanted {1} and got {2}"_s,
            { asset.fullName, formatInt(BMFontSupportedVersion), formatInt(header->version) });
        return false;
    }

    BMFontBlockHeader* blockHeader = nullptr;
    BMFontBlock_Common* common = nullptr;
    BMFont_Char* chars = nullptr;
    u32 charCount = 0;
    void const* pages = nullptr;

    blockHeader = (BMFontBlockHeader*)(data.data() + pos);
    pos += sizeof(BMFontBlockHeader);

    while (pos < data.size()) {
        switch (blockHeader->type) {
        case BMF_Block_Info: {
            // Ignored
        } break;

        case BMF_Block_Common: {
            common = (BMFontBlock_Common*)(data.data() + pos);
        } break;

        case BMF_Block_Pages: {
            pages = data.data() + pos;
        } break;

        case BMF_Block_Chars: {
            chars = (BMFont_Char*)(data.data() + pos);
            charCount = blockHeader->size / sizeof(BMFont_Char);
        } break;

        case BMF_Block_KerningPairs: {
            // TODO: Kerning!
        } break;
        }

        pos += blockHeader->size;

        blockHeader = (BMFontBlockHeader*)(data.data() + pos);
        pos += sizeof(BMFontBlockHeader);
    }

    if (!(common && chars && charCount && pages)) {
        // Something didn't load correctly!
        logError("BMFont file '{0}' seems to be lacking crucial data and could not be loaded!"_s, { asset.fullName });
    } else if (common->pageCount != 1) {
        logError("BMFont file '{0}' defines a font with {1} texture pages, but we require only 1."_s, { asset.fullName, formatInt(common->pageCount) });
    } else {
        BitmapFont* font = &asset.bitmapFont;
        font->m_line_height = common->lineHeight;
        font->m_base_y = common->base;
        font->m_glyph_count = 0;

        font->m_glyph_capacity = ceil_s32(charCount * 2.0f);
        smm glyphEntryMemorySize = font->m_glyph_capacity * sizeof(BitmapFontGlyphEntry);
        asset.data = assetsAllocate(&asset_manager(), glyphEntryMemorySize);
        font->m_glyph_entries = (BitmapFontGlyphEntry*)(asset.data.data());

        String textureName = String::from_null_terminated((char*)pages);
        font->m_texture = asset_manager().add_asset(AssetType::Texture, textureName);
        font->m_texture->ensure_is_loaded();

        float textureWidth = (float)font->m_texture->texture.surface->w;
        float textureHeight = (float)font->m_texture->texture.surface->h;

        for (u32 charIndex = 0;
            charIndex < charCount;
            charIndex++) {
            BMFont_Char* src = chars + charIndex;

            font->add_glyph(BitmapFontGlyph {
                .codepoint = static_cast<unichar>(src->id),
                .width = src->w,
                .height = src->h,
                .xOffset = src->xOffset,
                .yOffset = src->yOffset,
                .xAdvance = src->xAdvance,
                .uv = {
                    src->x / textureWidth,
                    src->y / textureHeight,
                    src->w / textureWidth,
                    src->h / textureHeight },
            });
        }
    }
    return true;
}

BitmapFontGlyphEntry* BitmapFont::find_glyph_entry(unichar target_char) const
{
    // Protect against div-0 error if this is the empty placeholder font
    if (m_glyph_capacity > 0) {
        u32 index = target_char % m_glyph_capacity;

        while (true) {
            BitmapFontGlyphEntry* entry = m_glyph_entries + index;
            if (entry->codepoint == target_char || !entry->isOccupied)
                return entry;

            index = (index + 1) % m_glyph_capacity;
        }
    }

    return nullptr;
}

void BitmapFont::add_glyph(BitmapFontGlyph&& glyph)
{
    BitmapFontGlyphEntry* result = find_glyph_entry(glyph.codepoint);

    ASSERT(result != nullptr); //, "Failed to add a glyph to font '{0}'!", {font->name});
    result->codepoint = glyph.codepoint;
    ASSERT(result->isOccupied == false); //, "Attempted to add glyph '{0}' to font '{1}' twice!", {formatInt(targetChar), font->name});
    result->isOccupied = true;
    result->glyph = glyph;

    m_glyph_count++;
}

BitmapFontGlyph* BitmapFont::find_glyph(unichar target_char) const
{
    if (auto* entry = find_glyph_entry(target_char))
        return &entry->glyph;

    logWarn("Failed to find char 0x{0} in font."_s, { formatInt(target_char, 16) });
    return nullptr;
}

V2I BitmapFont::calculate_text_size(StringView text, s32 max_width) const
{
    DEBUG_FUNCTION();

    ASSERT(max_width >= 0);

    V2I result = v2i(max_width, m_line_height);

    bool doWrap = (max_width > 0);
    s32 currentX = 0;
    s32 currentWordWidth = 0;
    s32 whitespaceWidthBeforeCurrentWord = 0;

    s32 lineCount = 1;
    s32 currentLineWidth = 0;
    s32 longestLineWidth = 0;

    s32 bytePos = 0;
    unichar c = 0;
    bool foundNext = getNextUnichar(text, &bytePos, &c);

    while (foundNext) {
        if (isNewline(c)) {
            bool prevWasCR = false;

            longestLineWidth = max(longestLineWidth, currentLineWidth + currentWordWidth + whitespaceWidthBeforeCurrentWord);

            whitespaceWidthBeforeCurrentWord = 0;
            currentWordWidth = 0;
            currentLineWidth = 0;

            currentX = 0;

            do {
                if (c == '\n' && prevWasCR) {
                    // It's a windows newline, and we already moved down one line, so skip it
                } else {
                    lineCount++;
                }

                prevWasCR = (c == '\r');
                foundNext = getNextUnichar(text, &bytePos, &c);
            } while (foundNext && isNewline(c));
        } else if (isWhitespace(c, false)) {
            // WHITESPACE LOOP

            // If we had a previous word, we know that it must have just finished, so add the whitespace!
            currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
            whitespaceWidthBeforeCurrentWord = 0;
            currentWordWidth = 0;

            unichar prevC = 0;
            BitmapFontGlyph* glyph = nullptr;

            do {
                if (c != prevC) {
                    glyph = find_glyph(c);
                    prevC = c;
                }

                if (glyph) {
                    whitespaceWidthBeforeCurrentWord += glyph->xAdvance;
                }

                foundNext = getNextUnichar(text, &bytePos, &c);
            } while (foundNext && isWhitespace(c, false));

            currentX += whitespaceWidthBeforeCurrentWord;

            continue;
        } else {
            BitmapFontGlyph* glyph = find_glyph(c);
            if (glyph) {
                if (doWrap && ((currentX + glyph->xAdvance) > max_width)) {
                    // In case this word is the only one on the line, AND is longer than
                    // the max width, we add currentWordWidth to the max() equation below!
                    // Before I added this, it got super weird.
                    // - Sam, 13/01/2020

                    longestLineWidth = max(longestLineWidth, currentLineWidth, currentWordWidth);

                    lineCount++;

                    if ((currentWordWidth + glyph->xAdvance) > max_width) {
                        // The current word is longer than will fit on an entire line!
                        // So, split it at the maximum line length.

                        // This should mean just wrapping the final character
                        currentWordWidth = 0;
                        currentX = 0;

                        currentLineWidth = 0;
                        whitespaceWidthBeforeCurrentWord = 0;
                    } else {
                        // Wrapping the whole word onto a new line
                        // Set the current position to where the next word will start
                        currentLineWidth = 0;
                        whitespaceWidthBeforeCurrentWord = 0;

                        currentX = currentWordWidth;
                    }
                }

                currentX += glyph->xAdvance;
                currentWordWidth += glyph->xAdvance;
            }
            foundNext = getNextUnichar(text, &bytePos, &c);
        }
    }

    // NB: Trailing whitespace can take currentX beyond max_width, because we don't adjust
    // the line width after whitespace. So, I'm clamping currentX to max_width to make sure
    // we don't erroneously return a size that includes the out-of-bounds whitespace.
    // Maybe we should just wrap it, but I think wrapping whitespace isn't generally what
    // you want - it's better to just start the new line with the next printable character.
    // - Sam, 15/01/2020
    result.x = max(longestLineWidth, doWrap ? min(currentX, max_width) : currentX);
    result.y = (m_line_height * lineCount);

    ASSERT(max_width == 0 || max_width >= result.x); // Somehow we measured text that's too wide!
    return result;
}

s32 BitmapFont::calculate_max_text_width(std::initializer_list<StringView> texts, s32 limit) const
{
    s32 result = 0;
    for (auto const& text : texts)
        result = max(result, calculate_text_size(text, limit).x);
    return result;
}

void _alignText(DrawRectsGroup* state, s32 startIndex, s32 endIndexInclusive, s32 lineWidth, s32 boundsWidth, Alignment align)
{
    if (lineWidth == 0) {
        return;
    }

    switch (align.horizontal) {
    case HAlign::Right: {
        s32 offsetX = boundsWidth - lineWidth;
        offsetRange(state, startIndex, endIndexInclusive, (float)offsetX, 0);
    } break;

    case HAlign::Centre: {
        s32 offsetX = (boundsWidth - lineWidth) / 2;
        offsetRange(state, startIndex, endIndexInclusive, (float)offsetX, 0);
    } break;

    case HAlign::Left: {
        // Nothing to do!
    } break;

    case HAlign::Fill: {
        // Nothing to do! (I think?)
    } break;

    default: {
        DEBUG_BREAK();
    } break;
    }
}

void drawText(RenderBuffer* renderBuffer, BitmapFont* font, StringView text, Rect2I bounds, Alignment align, Colour color, s8 shaderID, s32 caretIndex, DrawTextResult* caretInfoResult)
{
    DEBUG_FUNCTION();

    if (text.is_empty())
        return;

    ASSERT(renderBuffer != nullptr); // RenderBuffer must be provided!
    ASSERT(font != nullptr);         // Font must be provided!

    V2I topLeft = bounds.position();
    s32 maxWidth = bounds.width();

    //
    // NB: We *could* just use text.length here. That will over-estimate how many RenderItems to reserve,
    // which is fine if we're sticking with mostly English text, but in languages with a lot of multi-byte
    // characters, could involve reserving 2 or 3 times what we actually need.
    // countGlyphs() is not very fast, though it's less bad than I thought once I take the profiling out
    // of it. It accounts for about 4% of the time for drawText(), which is 0.1ms in my stress-test.
    // Also, the over-estimate will only boost the size of the renderitems array *once* as it never shrinks.
    // We could end up with one that's, I dunno, twice the size it needs to be... RenderItem_DrawThing is 64 bytes
    // right now, so 20,000 of them is around 1.25MB, which isn't a big deal.
    //
    // I think I'm going to go with the faster option for now, and maybe revisit this in the future.
    // Eventually we'll switch to unichar-strings, so all of this will be irrelevant anyway!
    //
    // - Sam, 28/06/2019
    //
    // s32 glyphsToOutput = countGlyphs(text.chars, text.length);
    s32 glyphsToOutput = text.length();

    DrawRectsGroup* group = beginRectsGroupForText(renderBuffer, font, shaderID, glyphsToOutput);

    s32 currentX = 0;
    s32 currentY = 0;
    bool doWrap = (maxWidth > 0);

    if (caretInfoResult && caretIndex == 0) {
        caretInfoResult->isValid = true;
        caretInfoResult->caretPosition = bounds.position() + v2i(currentX, currentY);
    }

    s32 startOfCurrentLine = 0;
    s32 currentLineWidth = 0;

    s32 whitespaceWidthBeforeCurrentWord = 0;

    s32 glyphCount = 0;
    s32 bytePos = 0;
    s32 currentChar = 0;

    unichar c = 0;
    unichar prevC = 0;
    BitmapFontGlyph* glyph = nullptr;

    bool foundNext = getNextUnichar(text, &bytePos, &c);
    while (foundNext) {
        if (isNewline(c)) {
            bool prevWasCR = false;

            // Do line-alignment stuff
            // (This only has to happen for the first newline in a series of newlines!)
            _alignText(group, startOfCurrentLine, glyphCount - 1, currentLineWidth, maxWidth, align);
            startOfCurrentLine = glyphCount;
            currentLineWidth = 0;

            currentX = 0;

            do {
                if (c == '\n' && prevWasCR) {
                    // It's a windows newline, and we already moved down one line, so skip it
                } else {
                    currentChar++;
                    if (caretInfoResult && currentChar == caretIndex) {
                        caretInfoResult->isValid = true;
                        caretInfoResult->caretPosition = bounds.position() + v2i(currentX, currentY);
                    }

                    currentY += font->line_height();
                }

                prevWasCR = (c == '\r');
                foundNext = getNextUnichar(text, &bytePos, &c);
            } while (foundNext && isNewline(c));
        } else if (isWhitespace(c, false)) {
            // NB: We don't handle whitespace characters that actually print something visibly.
            // Despite being an oxymoron, they do actually exist, but the chance of me actually
            // needing to use the "Ogham space mark" is rather slim, though I did add support
            // for it to isWhitespace() for the sake of correctness. I am strange.
            // But anyway, the additional complexity, as well as speed reduction, are not worth it.
            // - Sam, 10/07/2019

            do {
                if (c != prevC) {
                    glyph = font->find_glyph(c);
                    prevC = c;
                }

                if (glyph) {
                    whitespaceWidthBeforeCurrentWord += glyph->xAdvance;

                    currentChar++;
                    if (caretInfoResult && currentChar == caretIndex) {
                        caretInfoResult->isValid = true;
                        caretInfoResult->caretPosition = bounds.position() + v2i(currentX + whitespaceWidthBeforeCurrentWord, currentY);
                    }
                }
                foundNext = getNextUnichar(text, &bytePos, &c);
            } while (foundNext && isWhitespace(c, false));

            currentX += whitespaceWidthBeforeCurrentWord;
        } else {
            s32 startOfCurrentWord = glyphCount;
            s32 currentWordWidth = 0;

            do {
                if (c != prevC) {
                    glyph = font->find_glyph(c);
                    prevC = c;
                }

                if (glyph) {
                    if (doWrap && ((currentX + glyph->xAdvance) > maxWidth)) {
                        currentX = 0;
                        currentY += font->line_height();

                        if (currentWordWidth + glyph->xAdvance > maxWidth) {
                            // The current word is longer than will fit on an entire line!
                            // So, split it at the maximum line length.

                            // This should mean just wrapping the final character
                            startOfCurrentWord = glyphCount;
                            currentLineWidth = currentWordWidth;
                            currentWordWidth = 0;
                        } else if (currentWordWidth > 0) {
                            // Wrap the whole word onto a new line

                            // Offset from where the word was, to its new position
                            float offsetX = (float)-(currentLineWidth + whitespaceWidthBeforeCurrentWord);
                            float offsetY = (float)font->line_height();
                            offsetRange(group, startOfCurrentWord, glyphCount - 1, offsetX, offsetY);

                            // Set the current position to where the next word will start
                            currentX = currentWordWidth;
                        }

                        // Do line-alignment stuff
                        _alignText(group, startOfCurrentLine, startOfCurrentWord - 1, currentLineWidth, maxWidth, align);
                        startOfCurrentLine = startOfCurrentWord;
                        currentLineWidth = 0;
                        whitespaceWidthBeforeCurrentWord = 0;
                    }

                    addGlyphRect(group, glyph, v2(topLeft.x + currentX, topLeft.y + currentY), color);

                    currentChar++;
                    if (caretInfoResult && currentChar == caretIndex) {
                        caretInfoResult->isValid = true;
                        caretInfoResult->caretPosition = bounds.position() + v2i(currentX + glyph->xAdvance, currentY);
                    }

                    currentX += glyph->xAdvance;
                    currentWordWidth += glyph->xAdvance;

                    glyphCount++;
                }
                foundNext = getNextUnichar(text, &bytePos, &c);
            } while (foundNext && !isWhitespace(c, true));

            currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
            whitespaceWidthBeforeCurrentWord = 0;
        }
    }

    // Align the final line
    _alignText(group, startOfCurrentLine, glyphCount - 1, currentLineWidth, maxWidth, align);

    if (caretInfoResult && currentChar < caretIndex) {
        caretInfoResult->isValid = true;
        caretInfoResult->caretPosition = bounds.position() + v2i(currentX, currentY);
    }

    ASSERT(glyphCount == group->count);
    endRectsGroup(group);
}

V2I calculateTextPosition(V2I origin, V2I size, Alignment align)
{
    V2I offset;

    switch (align.horizontal) {
    case HAlign::Centre:
        offset.x = origin.x - (size.x / 2);
        break;
    case HAlign::Right:
        offset.x = origin.x - size.x;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Same as left
    default:
        offset.x = origin.x;
        break;
    }

    switch (align.vertical) {
    case VAlign::Centre:
        offset.y = origin.y - (size.y / 2);
        break;
    case VAlign::Bottom:
        offset.y = origin.y - size.y;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Same as top
    default:
        offset.y = origin.y;
        break;
    }

    return offset;
}
