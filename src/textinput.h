#pragma once

struct TextInput
{
	char *buffer;

	s32 byteLength;
	s32 glyphLength;

	s32 maxByteLength;

	s32 caretBytePos;
	s32 caretGlyphPos;
	f32 caretFlashCounter;
	f32 caretFlashCycleDuration; // Time for one on/off loop. If 0 this will hide the caret forever
};

TextInput newTextInput(MemoryArena *arena, s32 length, f32 caretFlashCycleDuration=1.0f);
// Returns true if pressed RETURN
bool updateTextInput(TextInput *textInput);
String textInputToString(TextInput *textInput);
Rect2I drawTextInput(struct RenderBuffer *renderBuffer, struct BitmapFont *font, TextInput *textInput, V2I origin, s32 align, V4 color, s32 maxWidth = 0);

void append(TextInput *textInput, char *source, s32 length);
void append(TextInput *textInput, String source);
void append(TextInput *textInput, char *source);
void insert(TextInput *textInput, String source);

void clear(TextInput *textInput);
void backspace(TextInput *textInput);
void deleteChar(TextInput *textInput);

void moveCaretLeft(TextInput *textInput, s32 count = 1);
void moveCaretRight(TextInput *textInput, s32 count = 1);
void moveCaretLeftWholeWord(TextInput *textInput);
void moveCaretRightWholeWord(TextInput *textInput);
