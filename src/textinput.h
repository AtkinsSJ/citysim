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

	String characterBlacklist;
};

TextInput newTextInput(MemoryArena *arena, s32 length, String characterBlacklist=nullString);
// Returns true if pressed RETURN
bool updateTextInput(TextInput *textInput);
String textInputToString(TextInput *textInput);
V2I calculateTextInputSize(TextInput *textInput, struct UITextInputStyle *style, s32 width = 0);
Rect2I drawTextInput(struct RenderBuffer *renderBuffer, TextInput *textInput, struct UITextInputStyle *style, Rect2I bounds);

void append(TextInput *textInput, char *source, s32 length);
void append(TextInput *textInput, String source);
void append(TextInput *textInput, char *source);
void insert(TextInput *textInput, String source);
void insert(TextInput *textInput, char c);

void clear(TextInput *textInput);
void backspace(TextInput *textInput);
void deleteChar(TextInput *textInput);

void moveCaretLeft(TextInput *textInput, s32 count = 1);
void moveCaretRight(TextInput *textInput, s32 count = 1);
void moveCaretLeftWholeWord(TextInput *textInput);
void moveCaretRightWholeWord(TextInput *textInput);
