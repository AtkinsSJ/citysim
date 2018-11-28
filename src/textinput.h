#pragma once

struct TextInput
{
	char *buffer;
	s32 length;
	s32 maxLength;

	s32 caretPos;
};

TextInput newTextInput(MemoryArena *arena, s32 length)
{
	TextInput b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.maxLength = length;

	return b;
}

String textInputToString(TextInput *textInput)
{
	String result = {};
	result.chars = textInput->buffer;
	result.length = textInput->length;

	return result;
}

void append(TextInput *textInput, char *source, s32 length)
{
	s32 lengthToCopy = length;
	if ((textInput->length + length) > textInput->maxLength)
	{
		lengthToCopy = textInput->maxLength - textInput->length;
	}

	for (s32 i=0; i < lengthToCopy; i++)
	{
		textInput->buffer[textInput->length++] = source[i];
	}

	// Final null, just in case.
	textInput->buffer[textInput->length] = 0;
	textInput->caretPos += lengthToCopy;
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
	append(textInput, source->buffer, source->length);
}

void insert(TextInput *textInput, char *source, s32 length)
{
	if (textInput->caretPos == textInput->length)
	{
		append(textInput, source, length);
		return;
	}

	s32 lengthToCopy = length;
	if ((textInput->length + length) > textInput->maxLength)
	{
		lengthToCopy = textInput->maxLength - textInput->length;
	}

	// move the existing chars by lengthToCopy
	for (s32 i=textInput->length - textInput->caretPos - 1; i>=0; i--)
	{
		textInput->buffer[textInput->caretPos + lengthToCopy + i] = textInput->buffer[textInput->caretPos + i];
	}

	// write from source
	for (s32 i=0; i < lengthToCopy; i++)
	{
		textInput->buffer[textInput->caretPos + i] = source[i];
	}

	textInput->length += lengthToCopy;
	textInput->caretPos += lengthToCopy;
}

void backspace(TextInput *textInput)
{
	if (textInput->caretPos > 0) 
	{
		textInput->caretPos--;

		// copy everything 1 to the left
		for (s32 i=textInput->caretPos; i<textInput->length-1; i++)
		{
			textInput->buffer[i] = textInput->buffer[i+1];
		}

		textInput->buffer[--textInput->length] = 0;
	}
}

void deleteChar(TextInput *textInput)
{
	if (textInput->caretPos < textInput->length) 
	{
		// copy everything 1 to the left
		for (s32 i=textInput->caretPos; i<textInput->length-1; i++)
		{
			textInput->buffer[i] = textInput->buffer[i+1];
		}

		textInput->buffer[--textInput->length] = 0;
	}
}

void clear(TextInput *textInput)
{
	textInput->length = 0;
	textInput->buffer[0] = 0;
	textInput->caretPos = 0;
}