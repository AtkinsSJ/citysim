

TextInput newTextInput(MemoryArena *arena, int32 length, real32 caretFlashCycleDuration=1.0f)
{
	TextInput b = {};
	b.buffer = PushArray(arena, char, length + 1);
	b.maxByteLength = length;
	b.caretFlashCycleDuration = caretFlashCycleDuration;

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

void moveCaretLeftWholeWord(TextInput *textInput)
{
	bool done = false;
	while (!done)
	{
		moveCaretLeft(textInput, 1);
		unichar glyph = readUnicodeChar(textInput->buffer + textInput->caretBytePos);
		if (isWhitespace(glyph))
		{
			done = true;
		}
		else if (textInput->caretBytePos == 0)
		{
			done = true;
		}
	}
}

void moveCaretRightWholeWord(TextInput *textInput)
{
	bool done = false;
	while (!done)
	{
		moveCaretRight(textInput, 1);
		unichar glyph = readUnicodeChar(textInput->buffer + textInput->caretBytePos);
		if (isWhitespace(glyph))
		{
			done = true;
		}
		else if (textInput->caretGlyphPos >= (textInput->glyphLength - 1))
		{
			done = true;
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

// Returns true if pressed RETURN
bool updateTextInput(TextInput *textInput, InputState *inputState)
{
	bool pressedReturn = false;

	if (keyJustPressed(inputState, SDLK_BACKSPACE, KeyMod_Ctrl))
	{
		clear(textInput);
		textInput->caretFlashCounter = 0;
	}
	else if (keyJustPressed(inputState, SDLK_BACKSPACE))
	{
		backspace(textInput);
		textInput->caretFlashCounter = 0;
	}
	if (keyJustPressed(inputState, SDLK_DELETE))
	{
		deleteChar(textInput);
		textInput->caretFlashCounter = 0;
	}

	if (keyJustPressed(inputState, SDLK_RETURN))
	{
		pressedReturn = true;
		textInput->caretFlashCounter = 0;
	}

	if (keyJustPressed(inputState, SDLK_LEFT))
	{
		if (modifierKeyIsPressed(inputState, KeyMod_Ctrl))
		{
			moveCaretLeftWholeWord(textInput);
		}
		else
		{
			moveCaretLeft(textInput, 1);
		}
		textInput->caretFlashCounter = 0;
	}
	else if (keyJustPressed(inputState, SDLK_RIGHT))
	{
		if (modifierKeyIsPressed(inputState, KeyMod_Ctrl))
		{
			moveCaretRightWholeWord(textInput);
		}
		else
		{
			moveCaretRight(textInput, 1);
		}
		moveCaretRight(textInput, 1);
		textInput->caretFlashCounter = 0;
	}
	if (keyJustPressed(inputState, SDLK_HOME))
	{
		textInput->caretBytePos = 0;
		textInput->caretGlyphPos = 0;
		textInput->caretFlashCounter = 0;
	}
	if (keyJustPressed(inputState, SDLK_END))
	{
		textInput->caretBytePos = textInput->byteLength;
		textInput->caretGlyphPos = textInput->glyphLength;
		textInput->caretFlashCounter = 0;
	}

	if (wasTextEntered(inputState))
	{
		char *enteredText = getEnteredText(inputState);
		int32 inputTextLength = strlen(enteredText);

		insert(textInput, enteredText, inputTextLength);
		textInput->caretFlashCounter = 0;
	}

	if (keyJustPressed(inputState, SDLK_v, KeyMod_Ctrl))
	{
		if (SDL_HasClipboardText())
		{
			char *clipboard = SDL_GetClipboardText();
			if (clipboard)
			{
				int32 clipboardLength = strlen(clipboard);
				insert(textInput, clipboard, clipboardLength);
				textInput->caretFlashCounter = 0;

				SDL_free(clipboard);
			}
		}
	}

	return pressedReturn;
}