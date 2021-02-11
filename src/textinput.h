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
V2I calculateTextInputSize(TextInput *textInput, struct UITextInputStyle *style, s32 maxWidth = 0, bool fillWidth = true);
Rect2I drawTextInput(struct RenderBuffer *renderBuffer, TextInput *textInput, struct UITextInputStyle *style, Rect2I bounds);

void append(TextInput *textInput, char *source, s32 length);
void append(TextInput *textInput, String source);
void insert(TextInput *textInput, String source);
void insert(TextInput *textInput, char c);

void clear(TextInput *textInput);
void backspaceChars(TextInput *textInput, s32 count = 1);
void deleteChars(TextInput *textInput, s32 count = 1);
void backspaceWholeWord(TextInput *textInput);
void deleteWholeWord(TextInput *textInput);

void moveCaretLeft(TextInput *textInput, s32 count = 1);
void moveCaretRight(TextInput *textInput, s32 count = 1);
void moveCaretLeftWholeWord(TextInput *textInput);
void moveCaretRightWholeWord(TextInput *textInput);

bool isEmpty(TextInput *textInput);

// Private
struct TextInputPos
{
	s32 bytePos;
	s32 glyphPos;
};
TextInputPos findStartOfWordLeft(TextInput *textInput);
TextInputPos findStartOfWordRight(TextInput *textInput);
