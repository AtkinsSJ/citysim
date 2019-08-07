

TextInput newTextInput(MemoryArena *arena, s32 length, f32 caretFlashCycleDuration)
{
	TextInput b = {};
	b.buffer = allocateArray<char>(arena, length + 1);
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

Rect2 drawTextInput(RenderBuffer *renderBuffer, BitmapFont *font, TextInput *textInput, V2 origin, s32 align, V4 color, f32 maxWidth)
{
	DEBUG_FUNCTION();

	String text = makeString(textInput->buffer, textInput->byteLength);

	V2 textSize  = calculateTextSize(font, text, maxWidth);
	V2 topLeft   = calculateTextPosition(origin, textSize, align);
	Rect2 bounds = rectPosSize(topLeft, textSize);

	// TODO: @Cleanup I really don't like DrawTextResult as it is now. This is the ONLY place we need it, and we keep
	// a bunch of data in it that we don't really need - we just want to know the top-left corner of where the caret should be.
	DrawTextResult drawTextResult = {};
	drawText(renderBuffer, font, text, bounds, align, color, renderer->shaderIds.text, textInput->caretGlyphPos, &drawTextResult);

	textInput->caretFlashCounter = (f32) fmod(textInput->caretFlashCounter + SECONDS_PER_FRAME, textInput->caretFlashCycleDuration);
	bool showCaret = (textInput->caretFlashCounter < (textInput->caretFlashCycleDuration * 0.5f));

	if (showCaret)
	{
		Rect2 caretRect = rectXYWH(topLeft.x, topLeft.y, 2, font->lineHeight);

		if (textInput->caretGlyphPos != 0 && drawTextResult.isValid)
		{
			// Draw it to the right of the glyph
			caretRect.pos = drawTextResult.caretPosition;
		}

		// Shifted 1px left for better legibility of text
		caretRect.x -= 1.0f;

		drawSingleRect(renderBuffer, caretRect, renderer->shaderIds.untextured, color);
	}

	return bounds;
}

void append(TextInput *textInput, char *source, s32 length)
{
	s32 bytesToCopy = length;
	if ((textInput->byteLength + length) > textInput->maxByteLength)
	{
		s32 newByteLengthToCopy = textInput->maxByteLength - textInput->byteLength;

		bytesToCopy = floorToWholeGlyphs(source, newByteLengthToCopy);
	}

	s32 glyphsToCopy = countGlyphs(source, bytesToCopy);

	for (s32 i=0; i < bytesToCopy; i++)
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
	append(textInput, makeString(source));
}

void insert(TextInput *textInput, String source)
{
	if (textInput->caretBytePos == textInput->byteLength)
	{
		append(textInput, source);
		return;
	}

	s32 bytesToCopy = source.length;
	if ((textInput->byteLength + source.length) > textInput->maxByteLength)
	{
		s32 newByteLengthToCopy = textInput->maxByteLength - textInput->byteLength;

		bytesToCopy = floorToWholeGlyphs(source.chars, newByteLengthToCopy);
	}

	s32 glyphsToCopy = countGlyphs(source.chars, bytesToCopy);

	// move the existing chars by bytesToCopy
	for (s32 i=textInput->byteLength - textInput->caretBytePos - 1; i>=0; i--)
	{
		textInput->buffer[textInput->caretBytePos + bytesToCopy + i] = textInput->buffer[textInput->caretBytePos + i];
	}

	// write from source
	for (s32 i=0; i < bytesToCopy; i++)
	{
		textInput->buffer[textInput->caretBytePos + i] = source.chars[i];
	}

	textInput->byteLength += bytesToCopy;
	textInput->caretBytePos += bytesToCopy;

	textInput->glyphLength += glyphsToCopy;
	textInput->caretGlyphPos += glyphsToCopy;
}

void moveCaretLeft(TextInput *textInput, s32 count)
{
	if (count < 1) return;

	if (textInput->caretGlyphPos > 0)
	{
		s32 toMove = min(count, textInput->caretGlyphPos);

		while (toMove--)
		{
			textInput->caretBytePos = findStartOfGlyph(textInput->buffer, textInput->caretBytePos - 1);
			textInput->caretGlyphPos--;
		}
	}
}

void moveCaretRight(TextInput *textInput, s32 count)
{
	if (count < 1) return;

	if (textInput->caretGlyphPos < textInput->glyphLength)
	{
		s32 toMove = min(count, textInput->glyphLength - textInput->caretGlyphPos);

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
		if (textInput->caretBytePos == 0)
		{
			done = true;
		}
		else if (isWhitespace(glyph))
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
		if (textInput->caretGlyphPos >= textInput->glyphLength)
		{
			done = true;
		}
		else if (isWhitespace(glyph))
		{
			done = true;
		}
	}
}

void backspace(TextInput *textInput)
{
	if (textInput->caretGlyphPos > 0) 
	{
		s32 oldBytePos = textInput->caretBytePos;

		moveCaretLeft(textInput, 1);

		s32 bytesToRemove = oldBytePos - textInput->caretBytePos;

		// copy everything bytesToRemove to the left
		for (s32 i = textInput->caretBytePos;
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
		s32 bytesToRemove = findStartOfNextGlyph(textInput->buffer, textInput->caretBytePos, textInput->maxByteLength) - textInput->caretBytePos;

		// copy everything bytesToRemove to the left
		for (s32 i = textInput->caretBytePos;
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
		insert(textInput, getEnteredText(inputState));
		textInput->caretFlashCounter = 0;
	}

	if (keyJustPressed(inputState, SDLK_v, KeyMod_Ctrl))
	{
		String clipboard = getClipboardText();
		if (!isEmpty(clipboard))
		{
			insert(textInput, clipboard);
		}
		textInput->caretFlashCounter = 0;
	}

	return pressedReturn;
}