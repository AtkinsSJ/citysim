/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "font.h"
#include <Debug/Debug.h>
#include <Gfx/Renderer.h>
#include <Util/Log.h>
#include <Util/Unicode.h>

BitmapFontGlyphEntry* findGlyphInternal(BitmapFont* font, unichar targetChar)
{
    BitmapFontGlyphEntry* result = nullptr;

    // Protect against div-0 error if this is the empty placeholder font
    if (font->glyphCapacity > 0) {
        u32 index = targetChar % font->glyphCapacity;

        while (true) {
            BitmapFontGlyphEntry* entry = font->glyphEntries + index;
            if (entry->codepoint == targetChar || !entry->isOccupied) {
                result = entry;
                break;
            }

            index = (index + 1) % font->glyphCapacity;
        }
    }

    return result;
}

BitmapFontGlyph* addGlyph(BitmapFont* font, unichar targetChar)
{
    BitmapFontGlyphEntry* result = findGlyphInternal(font, targetChar);

    ASSERT(result != nullptr); //, "Failed to add a glyph to font '{0}'!", {font->name});
    result->codepoint = targetChar;
    ASSERT(result->isOccupied == false); //, "Attempted to add glyph '{0}' to font '{1}' twice!", {formatInt(targetChar), font->name});
    result->isOccupied = true;

    font->glyphCount++;

    return &result->glyph;
}

BitmapFontGlyph* findGlyph(BitmapFont* font, unichar targetChar)
{
    BitmapFontGlyph* result = nullptr;
    BitmapFontGlyphEntry* entry = findGlyphInternal(font, targetChar);

    if (entry == nullptr) {
        logWarn("Failed to find char 0x{0} in font."_s, { formatInt(targetChar, 16) });
    } else {
        result = &entry->glyph;
    }

    return result;
}

V2I calculateTextSize(BitmapFont* font, String text, s32 maxWidth)
{
    DEBUG_FUNCTION();

    ASSERT(font != nullptr); // Font must be provided!
    ASSERT(maxWidth >= 0);

    V2I result = v2i(maxWidth, font->lineHeight);

    bool doWrap = (maxWidth > 0);
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
                    glyph = findGlyph(font, c);
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
            BitmapFontGlyph* glyph = findGlyph(font, c);
            if (glyph) {
                if (doWrap && ((currentX + glyph->xAdvance) > maxWidth)) {
                    // In case this word is the only one on the line, AND is longer than
                    // the max width, we add currentWordWidth to the max() equation below!
                    // Before I added this, it got super weird.
                    // - Sam, 13/01/2020

                    longestLineWidth = max(longestLineWidth, currentLineWidth, currentWordWidth);

                    lineCount++;

                    if ((currentWordWidth + glyph->xAdvance) > maxWidth) {
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

    // NB: Trailing whitespace can take currentX beyond maxWidth, because we don't adjust
    // the line width after whitespace. So, I'm clamping currentX to maxWidth to make sure
    // we don't erroneously return a size that includes the out-of-bounds whitespace.
    // Maybe we should just wrap it, but I think wrapping whitespace isn't generally what
    // you want - it's better to just start the new line with the next printable character.
    // - Sam, 15/01/2020
    result.x = max(longestLineWidth, doWrap ? min(currentX, maxWidth) : currentX);
    result.y = (font->lineHeight * lineCount);

    ASSERT(maxWidth == 0 || maxWidth >= result.x); // Somehow we measured text that's too wide!
    return result;
}

s32 calculateMaxTextWidth(BitmapFont* font, std::initializer_list<String> texts, s32 limit)
{
    s32 result = 0;

    for (auto text = texts.begin(); text != texts.end(); text++) {
        result = max(result, calculateTextSize(font, *text, limit).x);
    }

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

void drawText(RenderBuffer* renderBuffer, BitmapFont* font, String text, Rect2I bounds, Alignment align, Colour color, s8 shaderID, s32 caretIndex, DrawTextResult* caretInfoResult)
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
    s32 glyphsToOutput = text.length;

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

                    currentY += font->lineHeight;
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
                    glyph = findGlyph(font, c);
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
                    glyph = findGlyph(font, c);
                    prevC = c;
                }

                if (glyph) {
                    if (doWrap && ((currentX + glyph->xAdvance) > maxWidth)) {
                        currentX = 0;
                        currentY += font->lineHeight;

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
                            float offsetY = (float)font->lineHeight;
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
