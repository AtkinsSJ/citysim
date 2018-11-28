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
