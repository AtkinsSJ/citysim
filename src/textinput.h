#pragma once

struct TextInput
{
	char *buffer;

	int32 byteLength;
	int32 glyphLength;

	int32 maxByteLength;

	int32 caretBytePos;
	int32 caretGlyphPos;
};

TextInput newTextInput(MemoryArena *arena, int32 length)
{
	TextInput b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.maxByteLength = length;

	return b;
}

String textInputToString(TextInput *textInput)
{
	String result = {};
	result.chars = textInput->buffer;
	result.length = textInput->byteLength;

	return result;
}

void append(TextInput *textInput, char *source, int32 length)
{
	int32 bytesToCopy = length;
	if ((textInput->byteLength + length) > textInput->maxByteLength)
	{
		int32 newByteLengthToCopy = textInput->maxByteLength - textInput->byteLength;

		bytesToCopy = floorToWholeGlyphs(source, newByteLengthToCopy);
	}

	int32 glyphsToCopy = countGlyphs(source, bytesToCopy);

	for (int32 i=0; i < bytesToCopy; i++)
	{
		textInput->buffer[textInput->byteLength++] = source[i];
	}

	textInput->caretBytePos += bytesToCopy;

	textInput->caretGlyphPos += glyphsToCopy;
	textInput->glyphLength += glyphsToCopy;
}

void append(TextInput *textInput, String source)
{
	append(textInput, source.chars, source.length);
}

void append(TextInput *textInput, char *source)
{
	append(textInput, stringFromChars(source));
}

void append(TextInput *textInput, TextInput *source)
{
	append(textInput, source->buffer, source->byteLength);
}

void insert(TextInput *textInput, char *source, int32 length)
{
	if (textInput->caretBytePos == textInput->byteLength)
	{
		append(textInput, source, length);
		return;
	}

	int32 bytesToCopy = length;
	if ((textInput->byteLength + length) > textInput->maxByteLength)
	{
		int32 newByteLengthToCopy = textInput->maxByteLength - textInput->byteLength;

		bytesToCopy = floorToWholeGlyphs(source, newByteLengthToCopy);
	}

	int32 glyphsToCopy = countGlyphs(source, bytesToCopy);

	// move the existing chars by bytesToCopy
	for (int32 i=textInput->byteLength - textInput->caretBytePos - 1; i>=0; i--)
	{
		textInput->buffer[textInput->caretBytePos + bytesToCopy + i] = textInput->buffer[textInput->caretBytePos + i];
	}

	// write from source
	for (int32 i=0; i < bytesToCopy; i++)
	{
		textInput->buffer[textInput->caretBytePos + i] = source[i];
	}

	textInput->byteLength += bytesToCopy;
	textInput->caretBytePos += bytesToCopy;

	textInput->glyphLength += glyphsToCopy;
	textInput->caretGlyphPos += glyphsToCopy;
}

void moveCaretLeft(TextInput *textInput, int32 count = 1)
{
	if (count < 1) return;

	if (textInput->caretGlyphPos > 0)
	{
		int32 toMove = MIN(count, textInput->caretGlyphPos);

		while (toMove--)
		{
			textInput->caretBytePos = findStartOfGlyph(textInput->buffer, textInput->caretBytePos - 1);
			textInput->caretGlyphPos--;
		}
	}
}

void moveCaretRight(TextInput *textInput, int32 count = 1)
{
	if (count < 1) return;

	if (textInput->caretGlyphPos < textInput->glyphLength)
	{
		int32 toMove = MIN(count, textInput->glyphLength - textInput->caretGlyphPos);

		while (toMove--)
		{
			textInput->caretBytePos = findStartOfNextGlyph(textInput->buffer, textInput->caretBytePos, textInput->maxByteLength);
			textInput->caretGlyphPos++;
		}
	}
}

void backspace(TextInput *textInput)
{
	if (textInput->caretGlyphPos > 0) 
	{
		int32 oldBytePos = textInput->caretBytePos;

		moveCaretLeft(textInput, 1);

		int32 bytesToRemove = oldBytePos - textInput->caretBytePos;

		// copy everything bytesToRemove to the left
		for (int32 i = textInput->caretBytePos;
		     i < textInput->byteLength - bytesToRemove;
		     i++)
		{
			textInput->buffer[i] = textInput->buffer[i+bytesToRemove];
		}

		textInput->byteLength -= bytesToRemove;
		textInput->glyphLength--;
	}
}

void deleteChar(TextInput *textInput)
{
	if (textInput->caretGlyphPos < textInput->glyphLength) 
	{
		int32 bytesToRemove = findStartOfNextGlyph(textInput->buffer, textInput->caretBytePos, textInput->maxByteLength) - textInput->caretBytePos;

		// copy everything bytesToRemove to the left
		for (int32 i = textInput->caretBytePos;
		     i < textInput->byteLength - bytesToRemove;
		     i++)
		{
			textInput->buffer[i] = textInput->buffer[i+bytesToRemove];
		}

		textInput->byteLength -= bytesToRemove;
		textInput->glyphLength--;
	}
}

void clear(TextInput *textInput)
{
	textInput->byteLength = 0;
	textInput->glyphLength = 0;
	textInput->caretBytePos = 0;
	textInput->caretGlyphPos = 0;
}