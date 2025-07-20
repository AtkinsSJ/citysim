/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AppState.h"
#include "font.h"
#include "input.h"
#include "unicode.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <UI/Drawable.h>
#include <UI/TextInput.h>
#include <Util/Maths.h>
#include <Util/MemoryArena.h>

namespace UI {

bool TextInput::isEmpty()
{
    return byteLength == 0;
}

String TextInput::getLastWord()
{
    TextInputPos startOfWord = findStartOfWordLeft();
    String result = makeString(buffer + startOfWord.bytePos, caret.bytePos - startOfWord.bytePos);
    return result;
}

String TextInput::toString()
{
    return makeString(buffer, byteLength);
}

void TextInput::moveCaretLeft(s32 count)
{
    if (count < 1)
        return;

    if (caret.glyphPos > 0) {
        s32 toMove = min(count, caret.glyphPos);

        while (toMove--) {
            caret.bytePos = findStartOfGlyph(buffer, caret.bytePos - 1);
            caret.glyphPos--;
        }
    }
}

void TextInput::moveCaretRight(s32 count)
{
    if (count < 1)
        return;

    if (caret.glyphPos < glyphLength) {
        s32 toMove = min(count, glyphLength - caret.glyphPos);

        while (toMove--) {
            caret.bytePos = findStartOfNextGlyph(buffer, caret.bytePos, maxByteLength);
            caret.glyphPos++;
        }
    }
}

void TextInput::moveCaretLeftWholeWord()
{
    caret = findStartOfWordLeft();
}

void TextInput::moveCaretRightWholeWord()
{
    caret = findStartOfWordRight();
}

void TextInput::append(char* source, s32 length)
{
    s32 bytesToCopy = length;
    if ((byteLength + length) > maxByteLength) {
        s32 newByteLengthToCopy = maxByteLength - byteLength;

        bytesToCopy = floorToWholeGlyphs(source, newByteLengthToCopy);
    }

    s32 glyphsToCopy = countGlyphs(source, bytesToCopy);

    for (s32 i = 0; i < bytesToCopy; i++) {
        buffer[byteLength++] = source[i];
    }

    caret.bytePos += bytesToCopy;
    caret.glyphPos += glyphsToCopy;

    glyphLength += glyphsToCopy;
}

void TextInput::append(String source)
{
    append(source.chars, source.length);
}

void TextInput::insert(String source)
{
    if (caret.bytePos == byteLength) {
        append(source);
        return;
    }

    s32 bytesToCopy = source.length;
    if ((byteLength + source.length) > maxByteLength) {
        s32 newByteLengthToCopy = maxByteLength - byteLength;

        bytesToCopy = floorToWholeGlyphs(source.chars, newByteLengthToCopy);
    }

    s32 glyphsToCopy = countGlyphs(source.chars, bytesToCopy);

    // move the existing chars by bytesToCopy
    for (s32 i = byteLength - caret.bytePos - 1; i >= 0; i--) {
        buffer[caret.bytePos + bytesToCopy + i] = buffer[caret.bytePos + i];
    }

    // write from source
    for (s32 i = 0; i < bytesToCopy; i++) {
        buffer[caret.bytePos + i] = source.chars[i];
    }

    byteLength += bytesToCopy;
    caret.bytePos += bytesToCopy;

    glyphLength += glyphsToCopy;
    caret.glyphPos += glyphsToCopy;
}

void TextInput::insert(char c)
{
    insert(makeString(&c, 1));
}

void TextInput::clear()
{
    byteLength = 0;
    glyphLength = 0;
    caret.bytePos = 0;
    caret.glyphPos = 0;
}

void TextInput::backspaceChars(s32 count)
{
    if (caret.glyphPos > 0) {
        s32 charsToDelete = min(count, caret.glyphPos);

        s32 oldBytePos = caret.bytePos;

        moveCaretLeft(charsToDelete);

        s32 bytesToRemove = oldBytePos - caret.bytePos;

        // copy everything bytesToRemove to the left
        for (s32 i = caret.bytePos;
            i < byteLength - bytesToRemove;
            i++) {
            buffer[i] = buffer[i + bytesToRemove];
        }

        byteLength -= bytesToRemove;
        glyphLength -= charsToDelete;
    }
}

void TextInput::deleteChars(s32 count)
{
    s32 remainingChars = glyphLength - caret.glyphPos;
    if (remainingChars > 0) {
        s32 charsToDelete = min(count, remainingChars);

        // Move the caret so we can count the bytes
        TextInputPos oldCaret = caret;
        moveCaretRight(charsToDelete);

        s32 bytesToRemove = caret.bytePos - oldCaret.bytePos;

        // Move it back because we don't actually want it moved
        caret = oldCaret;

        // copy everything bytesToRemove to the left
        for (s32 i = caret.bytePos;
            i < byteLength - bytesToRemove;
            i++) {
            buffer[i] = buffer[i + bytesToRemove];
        }

        byteLength -= bytesToRemove;
        glyphLength -= charsToDelete;
    }
}

void TextInput::backspaceWholeWord()
{
    TextInputPos newCaretPos = findStartOfWordLeft();
    s32 toBackspace = caret.glyphPos - newCaretPos.glyphPos;

    backspaceChars(toBackspace);
}

void TextInput::deleteWholeWord()
{
    TextInputPos newCaretPos = findStartOfWordRight();
    s32 toDelete = newCaretPos.glyphPos - caret.glyphPos;

    deleteChars(toDelete);
}

TextInputPos TextInput::findStartOfWordLeft()
{
    TextInputPos result = caret;

    if (result.glyphPos > 0) {
        result.glyphPos--;
        result.bytePos = findStartOfGlyph(buffer, result.bytePos - 1);
    }

    while (result.glyphPos > 0) {
        s32 nextBytePos = findStartOfGlyph(buffer, result.bytePos - 1);

        unichar glyph = readUnicodeChar(buffer + nextBytePos);
        if (isWhitespace(glyph)) {
            break;
        }

        result.glyphPos--;
        result.bytePos = nextBytePos;
    }

    return result;
}

TextInputPos TextInput::findStartOfWordRight()
{
    TextInputPos result = caret;

    while (result.glyphPos < glyphLength) {
        result.glyphPos++;
        result.bytePos = findStartOfNextGlyph(buffer, result.bytePos, maxByteLength);

        unichar glyph = readUnicodeChar(buffer + result.bytePos);
        if (isWhitespace(glyph)) {
            break;
        }
    }

    return result;
}

// Returns true if pressed RETURN
bool updateTextInput(TextInput* textInput)
{
    bool pressedReturn = false;

    if (hasCapturedInput(textInput)) {
        if (keyJustPressed(SDLK_BACKSPACE, KeyMod_Ctrl)) {
            textInput->backspaceWholeWord();
            textInput->caretFlashCounter = 0;
        } else if (keyJustPressed(SDLK_BACKSPACE)) {
            textInput->backspaceChars(1);
            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_DELETE, KeyMod_Ctrl)) {
            textInput->deleteWholeWord();
            textInput->caretFlashCounter = 0;
        } else if (keyJustPressed(SDLK_DELETE)) {
            textInput->deleteChars(1);
            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_RETURN)) {
            pressedReturn = true;
            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_LEFT)) {
            if (modifierKeyIsPressed(KeyMod_Ctrl)) {
                textInput->moveCaretLeftWholeWord();
            } else {
                textInput->moveCaretLeft(1);
            }
            textInput->caretFlashCounter = 0;
        } else if (keyJustPressed(SDLK_RIGHT)) {
            if (modifierKeyIsPressed(KeyMod_Ctrl)) {
                textInput->moveCaretRightWholeWord();
            } else {
                textInput->moveCaretRight(1);
            }
            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_HOME)) {
            textInput->caret.bytePos = 0;
            textInput->caret.glyphPos = 0;
            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_END)) {
            textInput->caret.bytePos = textInput->byteLength;
            textInput->caret.glyphPos = textInput->glyphLength;
            textInput->caretFlashCounter = 0;
        }

        if (wasTextEntered()) {
            // Filter the input to remove any blacklisted characters
            String enteredText = getEnteredText();
            for (s32 charIndex = 0; charIndex < enteredText.length; charIndex++) {
                char c = enteredText[charIndex];
                if (!contains(textInput->characterBlacklist, c)) {
                    textInput->insert(c);
                }
            }

            textInput->caretFlashCounter = 0;
        }

        if (keyJustPressed(SDLK_v, KeyMod_Ctrl)) {
            // Filter the input to remove any blacklisted characters
            String enteredText = getClipboardText();
            for (s32 charIndex = 0; charIndex < enteredText.length; charIndex++) {
                char c = enteredText[charIndex];
                if (!contains(textInput->characterBlacklist, c)) {
                    textInput->insert(c);
                }
            }

            textInput->caretFlashCounter = 0;
        }
    }

    return pressedReturn;
}

TextInput newTextInput(MemoryArena* arena, s32 length, String characterBlacklist)
{
    TextInput b = {};
    b.buffer = arena->allocate_multiple<char>(length + 1);
    b.maxByteLength = length;
    b.characterBlacklist = characterBlacklist;

    return b;
}

V2I calculateTextInputSize(TextInput* textInput, TextInputStyle* style, s32 maxWidth, bool fillWidth)
{
    s32 textMaxWidth = (maxWidth == 0) ? 0 : (maxWidth - (style->padding.left + style->padding.right));

    BitmapFont* font = getFont(&style->font);
    String text = textInput->toString();
    V2I textSize = calculateTextSize(font, text, textMaxWidth);

    s32 resultWidth = 0;

    if (fillWidth && (maxWidth > 0)) {
        resultWidth = maxWidth;
    } else {
        resultWidth = (textSize.x + style->padding.left + style->padding.right);
    }

    V2I result = v2i(resultWidth, textSize.y + style->padding.top + style->padding.bottom);

    return result;
}

Rect2I drawTextInput(RenderBuffer* renderBuffer, TextInput* textInput, TextInputStyle* style, Rect2I bounds)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    auto& renderer = the_renderer();
    String text = textInput->toString();
    BitmapFont* font = getFont(&style->font);

    Drawable(&style->background).draw(renderBuffer, bounds);

    bool showCaret = style->showCaret
        && hasCapturedInput(textInput)
        && (textInput->caretFlashCounter < (style->caretFlashCycleDuration * 0.5f));

    Rect2I textBounds = shrink(bounds, style->padding);
    DrawTextResult drawTextResult = {};
    drawText(renderBuffer, font, text, textBounds, style->textAlignment, style->textColor, renderer.shaderIds.text, textInput->caret.glyphPos, &drawTextResult);

    textInput->caretFlashCounter = (f32)fmod(textInput->caretFlashCounter + AppState::the().deltaTime, style->caretFlashCycleDuration);

    if (showCaret) {
        Rect2 caretRect = rectXYWHi(textBounds.x, textBounds.y, 2, font->lineHeight);

        if (textInput->caret.glyphPos != 0 && drawTextResult.isValid) {
            // Draw it to the right of the glyph
            caretRect.pos = v2(drawTextResult.caretPosition);
        }

        // Shifted 1px left for better legibility of text
        caretRect.x -= 1.0f;

        drawSingleRect(renderBuffer, caretRect, renderer.shaderIds.untextured, style->textColor);
    }

    return bounds;
}

}
