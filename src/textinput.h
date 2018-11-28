#pragma once

struct TextInput
{
	char *buffer;
	s32 length;
	s32 maxLength;
	s32 caretPos;
TextInput newTextInput(MemoryArena *arena, s32 length)
void append(TextInput *textInput, char *source, s32 length)
	s32 lengthToCopy = length;
	for (s32 i=0; i < lengthToCopy; i++)
void insert(TextInput *textInput, char *source, s32 length)

	s32 lengthToCopy = length;
	int32 glyphLength;
	for (s32 i=textInput->length - textInput->caretPos - 1; i>=0; i--)
	for (s32 i=0; i < lengthToCopy; i++)
		for (s32 i=textInput->caretPos; i<textInput->length-1; i++)
		for (s32 i=textInput->caretPos; i<textInput->length-1; i++)

	int32 maxByteLength;

	int32 caretBytePos;
	int32 caretGlyphPos;
	real32 caretFlashCounter;
	real32 caretFlashCycleDuration; // Time for one on/off loop. If 0 this will hide the caret forever
};