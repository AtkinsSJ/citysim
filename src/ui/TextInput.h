/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../util/Forward.h"
#include "Forward.h"

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
    f32 caretFlashCounter;

    String characterBlacklist;

    // Methods
    bool isEmpty();

    String getLastWord();
    String toString();

    void moveCaretLeft(s32 count);
    void moveCaretRight(s32 count);
    void moveCaretLeftWholeWord();
    void moveCaretRightWholeWord();

    void append(char* source, s32 length);
    void append(String source);
    void insert(String source);
    void insert(char c);

    void clear();
    void backspaceChars(s32 count);
    void deleteChars(s32 count);
    void backspaceWholeWord();
    void deleteWholeWord();

    // Internal
    TextInputPos findStartOfWordLeft();
    TextInputPos findStartOfWordRight();
};

TextInput newTextInput(MemoryArena* arena, s32 length, String characterBlacklist = "`"_s);
// Returns true if pressed RETURN
bool updateTextInput(TextInput* textInput);
V2I calculateTextInputSize(TextInput* textInput, struct UI::TextInputStyle* style, s32 maxWidth = 0, bool fillWidth = true);
Rect2I drawTextInput(struct RenderBuffer* renderBuffer, TextInput* textInput, struct UI::TextInputStyle* style, Rect2I bounds);

}
