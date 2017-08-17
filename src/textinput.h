#pragma once

struct TextInput
{
	char *buffer;

	int32 byteLength;
	int32 glyphLength;

	int32 maxByteLength;

	int32 caretBytePos;
	int32 caretGlyphPos;
	real32 caretFlashCounter;
	real32 caretFlashCycleDuration; // Time for one on/off loop. If 0 this will hide the caret forever
};