

TextInput newTextInput(MemoryArena *arena, s32 length, String characterBlacklist)
{
	TextInput b = {};
	b.buffer = allocateMultiple<char>(arena, length + 1);
	b.maxByteLength = length;
	b.characterBlacklist = characterBlacklist;

	return b;
}

String textInputToString(TextInput *textInput)
{
	return makeString(textInput->buffer, textInput->byteLength);
}

V2I calculateTextInputSize(TextInput *textInput, UITextInputStyle *style, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (style->padding * 2);
	s32 textMaxWidth = (maxWidth == 0) ? 0 : (maxWidth - doublePadding);

	BitmapFont *font = getFont(&style->font);
	String text = textInputToString(textInput);
	V2I textSize = calculateTextSize(font, text, textMaxWidth);

	s32 resultWidth = 0;

	if (fillWidth && (maxWidth > 0))
	{
		resultWidth = maxWidth;
	}
	else
	{
		resultWidth = (textSize.x + doublePadding);
	}

	V2I result = v2i(resultWidth, textSize.y + doublePadding);

	return result;
}

Rect2I drawTextInput(RenderBuffer *renderBuffer, TextInput *textInput, UITextInputStyle *style, Rect2I bounds)
{
	DEBUG_FUNCTION();

	String text = textInputToString(textInput);
	BitmapFont *font = getFont(&style->font);

	Rect2I textBounds = shrink(bounds, style->padding);

	drawSingleRect(renderBuffer, bounds, renderer->shaderIds.untextured, style->backgroundColor);

	bool showCaret = style->showCaret
					 && hasCapturedInput(textInput)
					 && (textInput->caretFlashCounter < (style->caretFlashCycleDuration * 0.5f));

	DrawTextResult drawTextResult = {};
	drawText(renderBuffer, font, text, textBounds, style->textAlignment, style->textColor, renderer->shaderIds.text, textInput->caretGlyphPos, &drawTextResult);

	textInput->caretFlashCounter = (f32) fmod(textInput->caretFlashCounter + SECONDS_PER_FRAME, style->caretFlashCycleDuration);

	if (showCaret)
	{
		Rect2 caretRect = rectXYWHi(textBounds.x, textBounds.y, 2, font->lineHeight);

		if (textInput->caretGlyphPos != 0 && drawTextResult.isValid)
		{
			// Draw it to the right of the glyph
			caretRect.pos = v2(drawTextResult.caretPosition);
		}

		// Shifted 1px left for better legibility of text
		caretRect.x -= 1.0f;

		drawSingleRect(renderBuffer, caretRect, renderer->shaderIds.untextured, style->textColor);
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

void insert(TextInput *textInput, char c)
{
	insert(textInput, makeString(&c, 1));
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
bool updateTextInput(TextInput *textInput)
{
	bool pressedReturn = false;

	if (hasCapturedInput(textInput))
	{
		if (keyJustPressed(SDLK_BACKSPACE, KeyMod_Ctrl))
		{
			clear(textInput);
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_BACKSPACE))
		{
			backspace(textInput);
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_DELETE))
		{
			deleteChar(textInput);
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_RETURN))
		{
			pressedReturn = true;
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_LEFT))
		{
			if (modifierKeyIsPressed(KeyMod_Ctrl))
			{
				moveCaretLeftWholeWord(textInput);
			}
			else
			{
				moveCaretLeft(textInput, 1);
			}
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_RIGHT))
		{
			if (modifierKeyIsPressed(KeyMod_Ctrl))
			{
				moveCaretRightWholeWord(textInput);
			}
			else
			{
				moveCaretRight(textInput, 1);
			}
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_HOME))
		{
			textInput->caretBytePos = 0;
			textInput->caretGlyphPos = 0;
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_END))
		{
			textInput->caretBytePos = textInput->byteLength;
			textInput->caretGlyphPos = textInput->glyphLength;
			textInput->caretFlashCounter = 0;
		}

		if (wasTextEntered())
		{
			// Filter the input to remove any blacklisted characters
			String enteredText = getEnteredText();
			for (s32 charIndex=0; charIndex < enteredText.length; charIndex++)
			{
				char c = enteredText[charIndex];
				if (!contains(textInput->characterBlacklist, c))
				{
					insert(textInput, c);
				}
			}

			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_v, KeyMod_Ctrl))
		{
			// Filter the input to remove any blacklisted characters
			String enteredText = getClipboardText();
			for (s32 charIndex=0; charIndex < enteredText.length; charIndex++)
			{
				char c = enteredText[charIndex];
				if (!contains(textInput->characterBlacklist, c))
				{
					insert(textInput, c);
				}
			}
			
			textInput->caretFlashCounter = 0;
		}
	}

	return pressedReturn;
}

inline bool isEmpty(TextInput *textInput)
{
	return textInput->byteLength == 0;
}
