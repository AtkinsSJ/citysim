/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Forward.h>
#include <UI/Forward.h>
#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/String.h>

namespace UI {

struct TextInputPos {
    s32 bytePos;
    s32 glyphPos;
};

struct TextInput {

    char* buffer;

    s32 byteLength;
    s32 glyphLength;

    s32 maxByteLength;

    TextInputPos caret;
    float caretFlashCounter;

    String characterBlacklist;

    // Methods
    bool isEmpty() const;

    StringView last_word() const;
    String toString() const;
    StringView text() const;

    void moveCaretLeft(s32 count);
    void moveCaretRight(s32 count);
    void moveCaretLeftWholeWord();
    void moveCaretRightWholeWord();

    void append(StringView source);
    void append(char c);
    void insert(StringView source);
    void insert(char c);

    void clear();
    void backspaceChars(s32 count);
    void deleteChars(s32 count);
    void backspaceWholeWord();
    void deleteWholeWord();

    // Internal
    TextInputPos findStartOfWordLeft() const;
    TextInputPos findStartOfWordRight() const;
};

TextInput newTextInput(MemoryArena* arena, s32 length, String characterBlacklist = "`"_s);
// Returns true if pressed RETURN
bool updateTextInput(TextInput* textInput);
V2I calculateTextInputSize(TextInput* textInput, TextInputStyle* style, s32 maxWidth = 0, bool fillWidth = true);
Rect2I drawTextInput(RenderBuffer* renderBuffer, TextInput* textInput, TextInputStyle* style, Rect2I bounds);

}
