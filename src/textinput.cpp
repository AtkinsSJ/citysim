

TextInput newTextInput(MemoryArena *arena, s32 length, String characterBlacklist)
{
	TextInput b = {};
	b.buffer = allocateMultiple<char>(arena, length + 1);
	b.maxByteLength = length;
	b.characterBlacklist = characterBlacklist;

	return b;
}

// Returns true if pressed RETURN
bool updateTextInput(TextInput *textInput)
{
	bool pressedReturn = false;

	if (hasCapturedInput(textInput))
	{
		if (keyJustPressed(SDLK_BACKSPACE, KeyMod_Ctrl))
		{
			backspaceWholeWord(textInput);
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_BACKSPACE))
		{
			backspaceChars(textInput);
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_DELETE, KeyMod_Ctrl))
		{
			deleteWholeWord(textInput);
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_DELETE))
		{
			deleteChars(textInput);
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


	UIDrawable(&style->background).draw(renderBuffer, bounds);

	bool showCaret = style->showCaret
					 && hasCapturedInput(textInput)
					 && (textInput->caretFlashCounter < (style->caretFlashCycleDuration * 0.5f));

	Rect2I textBounds = shrink(bounds, style->padding);
	DrawTextResult drawTextResult = {};
	drawText(renderBuffer, font, text, textBounds, style->textAlignment, style->textColor, renderer->shaderIds.text, textInput->caretGlyphPos, &drawTextResult);

	textInput->caretFlashCounter = (f32) fmod(textInput->caretFlashCounter + globalAppState.deltaTime, style->caretFlashCycleDuration);

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

inline void append(TextInput *textInput, String source)
{
	append(textInput, source.chars, source.length);
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

inline void insert(TextInput *textInput, char c)
{
	insert(textInput, makeString(&c, 1));
}

void clear(TextInput *textInput)
{
	textInput->byteLength = 0;
	textInput->glyphLength = 0;
	textInput->caretBytePos = 0;
	textInput->caretGlyphPos = 0;
}

void backspaceChars(TextInput *textInput, s32 count)
{
	if (textInput->caretGlyphPos > 0) 
	{
		s32 charsToDelete = min(count, textInput->caretGlyphPos);

		s32 oldBytePos = textInput->caretBytePos;

		moveCaretLeft(textInput, charsToDelete);

		s32 bytesToRemove = oldBytePos - textInput->caretBytePos;

		// copy everything bytesToRemove to the left
		for (s32 i = textInput->caretBytePos;
		     i < textInput->byteLength - bytesToRemove;
		     i++)
		{
			textInput->buffer[i] = textInput->buffer[i+bytesToRemove];
		}

		textInput->byteLength -= bytesToRemove;
		textInput->glyphLength -= charsToDelete;
	}
}

void deleteChars(TextInput *textInput, s32 count)
{
	s32 remainingChars = textInput->glyphLength - textInput->caretGlyphPos;
	if (remainingChars > 0) 
	{
		s32 charsToDelete = min(count, remainingChars);

		// Move the caret so we can count the bytes
		s32 oldBytePos = textInput->caretBytePos;
		s32 oldGlyphPos = textInput->caretGlyphPos;
		moveCaretRight(textInput, charsToDelete);

		s32 bytesToRemove = textInput->caretBytePos - oldBytePos;

		// Move it back because we don't actually want it moved
		textInput->caretBytePos = oldBytePos;
		textInput->caretGlyphPos = oldGlyphPos;

		// copy everything bytesToRemove to the left
		for (s32 i = textInput->caretBytePos;
		     i < textInput->byteLength - bytesToRemove;
		     i++)
		{
			textInput->buffer[i] = textInput->buffer[i+bytesToRemove];
		}

		textInput->byteLength -= bytesToRemove;
		textInput->glyphLength -= charsToDelete;
	}
}

void backspaceWholeWord(TextInput *textInput)
{
	TextInputPos newCaretPos = findStartOfWordLeft(textInput);
	s32 toBackspace = textInput->caretGlyphPos - newCaretPos.glyphPos;

	backspaceChars(textInput, toBackspace);
}

void deleteWholeWord(TextInput *textInput)
{
	TextInputPos newCaretPos = findStartOfWordRight(textInput);
	s32 toDelete = newCaretPos.glyphPos - textInput->caretGlyphPos;

	deleteChars(textInput, toDelete);
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

inline void moveCaretLeftWholeWord(TextInput *textInput)
{
	TextInputPos newCaretPos = findStartOfWordLeft(textInput);
	textInput->caretBytePos  = newCaretPos.bytePos;
	textInput->caretGlyphPos = newCaretPos.glyphPos;
}

void moveCaretRightWholeWord(TextInput *textInput)
{
	TextInputPos newCaretPos = findStartOfWordRight(textInput);
	textInput->caretBytePos  = newCaretPos.bytePos;
	textInput->caretGlyphPos = newCaretPos.glyphPos;
}

inline bool isEmpty(TextInput *textInput)
{
	return textInput->byteLength == 0;
}

TextInputPos findStartOfWordLeft(TextInput *textInput)
{
	TextInputPos result = {textInput->caretBytePos, textInput->caretGlyphPos};

	if (result.glyphPos > 0)
	{
		result.glyphPos--;
		result.bytePos = findStartOfGlyph(textInput->buffer, result.bytePos - 1);
	}

	while (result.glyphPos > 0)
	{
		s32 nextBytePos = findStartOfGlyph(textInput->buffer, result.bytePos - 1);

		unichar glyph = readUnicodeChar(textInput->buffer + nextBytePos);
		if (isWhitespace(glyph))
		{
			break;
		}

		result.glyphPos--;
		result.bytePos = nextBytePos;
	}

	return result;
}

TextInputPos findStartOfWordRight(TextInput *textInput)
{
	TextInputPos result = {textInput->caretBytePos, textInput->caretGlyphPos};

	while (result.glyphPos < textInput->glyphLength)
	{
		result.glyphPos++;
		result.bytePos = findStartOfNextGlyph(textInput->buffer, result.bytePos, textInput->maxByteLength);

		unichar glyph = readUnicodeChar(textInput->buffer + result.bytePos);
		if (isWhitespace(glyph))
		{
			break;
		}
	}

	return result;
}
