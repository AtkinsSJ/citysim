
inline bool TextInput::isEmpty()
{
	return byteLength == 0;
}

String TextInput::getLastWord()
{
	TextInputPos startOfWord = findStartOfWordLeft();
	String result = makeString(buffer + startOfWord.bytePos, caretBytePos - startOfWord.bytePos);
	return result;
}

inline String TextInput::toString()
{
	return makeString(buffer, byteLength);
}

void TextInput::moveCaretLeft(s32 count)
{
	if (count < 1) return;

	if (caretGlyphPos > 0)
	{
		s32 toMove = min(count, caretGlyphPos);

		while (toMove--)
		{
			caretBytePos = findStartOfGlyph(buffer, caretBytePos - 1);
			caretGlyphPos--;
		}
	}
}

void TextInput::moveCaretRight(s32 count)
{
	if (count < 1) return;

	if (caretGlyphPos < glyphLength)
	{
		s32 toMove = min(count, glyphLength - caretGlyphPos);

		while (toMove--)
		{
			caretBytePos = findStartOfNextGlyph(buffer, caretBytePos, maxByteLength);
			caretGlyphPos++;
		}
	}
}

void TextInput::moveCaretLeftWholeWord()
{
	TextInputPos newCaretPos = findStartOfWordLeft();
	caretBytePos  = newCaretPos.bytePos;
	caretGlyphPos = newCaretPos.glyphPos;
}

void TextInput::moveCaretRightWholeWord()
{
	TextInputPos newCaretPos = findStartOfWordRight();
	caretBytePos  = newCaretPos.bytePos;
	caretGlyphPos = newCaretPos.glyphPos;
}


void TextInput::append(char *source, s32 length)
{
	s32 bytesToCopy = length;
	if ((byteLength + length) > maxByteLength)
	{
		s32 newByteLengthToCopy = maxByteLength - byteLength;

		bytesToCopy = floorToWholeGlyphs(source, newByteLengthToCopy);
	}

	s32 glyphsToCopy = countGlyphs(source, bytesToCopy);

	for (s32 i=0; i < bytesToCopy; i++)
	{
		buffer[byteLength++] = source[i];
	}

	caretBytePos += bytesToCopy;

	caretGlyphPos += glyphsToCopy;
	glyphLength += glyphsToCopy;
}

inline void TextInput::append(String source)
{
	append(source.chars, source.length);
}

void TextInput::insert(String source)
{
	if (caretBytePos == byteLength)
	{
		append(source);
		return;
	}

	s32 bytesToCopy = source.length;
	if ((byteLength + source.length) > maxByteLength)
	{
		s32 newByteLengthToCopy = maxByteLength - byteLength;

		bytesToCopy = floorToWholeGlyphs(source.chars, newByteLengthToCopy);
	}

	s32 glyphsToCopy = countGlyphs(source.chars, bytesToCopy);

	// move the existing chars by bytesToCopy
	for (s32 i=byteLength - caretBytePos - 1; i>=0; i--)
	{
		buffer[caretBytePos + bytesToCopy + i] = buffer[caretBytePos + i];
	}

	// write from source
	for (s32 i=0; i < bytesToCopy; i++)
	{
		buffer[caretBytePos + i] = source.chars[i];
	}

	byteLength += bytesToCopy;
	caretBytePos += bytesToCopy;

	glyphLength += glyphsToCopy;
	caretGlyphPos += glyphsToCopy;
}

inline void TextInput::insert(char c)
{
	insert(makeString(&c, 1));
}

void TextInput::clear()
{
	byteLength = 0;
	glyphLength = 0;
	caretBytePos = 0;
	caretGlyphPos = 0;
}

void TextInput::backspaceChars(s32 count)
{
	if (caretGlyphPos > 0) 
	{
		s32 charsToDelete = min(count, caretGlyphPos);

		s32 oldBytePos = caretBytePos;

		moveCaretLeft(charsToDelete);

		s32 bytesToRemove = oldBytePos - caretBytePos;

		// copy everything bytesToRemove to the left
		for (s32 i = caretBytePos;
		     i < byteLength - bytesToRemove;
		     i++)
		{
			buffer[i] = buffer[i+bytesToRemove];
		}

		byteLength -= bytesToRemove;
		glyphLength -= charsToDelete;
	}
}

void TextInput::deleteChars(s32 count)
{
	s32 remainingChars = glyphLength - caretGlyphPos;
	if (remainingChars > 0) 
	{
		s32 charsToDelete = min(count, remainingChars);

		// Move the caret so we can count the bytes
		s32 oldBytePos = caretBytePos;
		s32 oldGlyphPos = caretGlyphPos;
		moveCaretRight(charsToDelete);

		s32 bytesToRemove = caretBytePos - oldBytePos;

		// Move it back because we don't actually want it moved
		caretBytePos = oldBytePos;
		caretGlyphPos = oldGlyphPos;

		// copy everything bytesToRemove to the left
		for (s32 i = caretBytePos;
		     i < byteLength - bytesToRemove;
		     i++)
		{
			buffer[i] = buffer[i+bytesToRemove];
		}

		byteLength -= bytesToRemove;
		glyphLength -= charsToDelete;
	}
}

void TextInput::backspaceWholeWord()
{
	TextInputPos newCaretPos = findStartOfWordLeft();
	s32 toBackspace = caretGlyphPos - newCaretPos.glyphPos;

	backspaceChars(toBackspace);
}

void TextInput::deleteWholeWord()
{
	TextInputPos newCaretPos = findStartOfWordRight();
	s32 toDelete = newCaretPos.glyphPos - caretGlyphPos;

	deleteChars(toDelete);
}


TextInput::TextInputPos TextInput::findStartOfWordLeft()
{
	TextInputPos result = {caretBytePos, caretGlyphPos};

	if (result.glyphPos > 0)
	{
		result.glyphPos--;
		result.bytePos = findStartOfGlyph(buffer, result.bytePos - 1);
	}

	while (result.glyphPos > 0)
	{
		s32 nextBytePos = findStartOfGlyph(buffer, result.bytePos - 1);

		unichar glyph = readUnicodeChar(buffer + nextBytePos);
		if (isWhitespace(glyph))
		{
			break;
		}

		result.glyphPos--;
		result.bytePos = nextBytePos;
	}

	return result;
}

TextInput::TextInputPos TextInput::findStartOfWordRight()
{
	TextInputPos result = {caretBytePos, caretGlyphPos};

	while (result.glyphPos < glyphLength)
	{
		result.glyphPos++;
		result.bytePos = findStartOfNextGlyph(buffer, result.bytePos, maxByteLength);

		unichar glyph = readUnicodeChar(buffer + result.bytePos);
		if (isWhitespace(glyph))
		{
			break;
		}
	}

	return result;
}

// Returns true if pressed RETURN
bool UI::updateTextInput(TextInput *textInput)
{
	bool pressedReturn = false;

	if (hasCapturedInput(textInput))
	{
		if (keyJustPressed(SDLK_BACKSPACE, KeyMod_Ctrl))
		{
			textInput->backspaceWholeWord();
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_BACKSPACE))
		{
			textInput->backspaceChars(1);
			textInput->caretFlashCounter = 0;
		}

		if (keyJustPressed(SDLK_DELETE, KeyMod_Ctrl))
		{
			textInput->deleteWholeWord();
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_DELETE))
		{
			textInput->deleteChars(1);
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
				textInput->moveCaretLeftWholeWord();
			}
			else
			{
				textInput->moveCaretLeft(1);
			}
			textInput->caretFlashCounter = 0;
		}
		else if (keyJustPressed(SDLK_RIGHT))
		{
			if (modifierKeyIsPressed(KeyMod_Ctrl))
			{
				textInput->moveCaretRightWholeWord();
			}
			else
			{
				textInput->moveCaretRight(1);
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
					textInput->insert(c);
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
					textInput->insert(c);
				}
			}
			
			textInput->caretFlashCounter = 0;
		}
	}

	return pressedReturn;
}

TextInput UI::newTextInput(MemoryArena *arena, s32 length, String characterBlacklist)
{
	TextInput b = {};
	b.buffer = allocateMultiple<char>(arena, length + 1);
	b.maxByteLength = length;
	b.characterBlacklist = characterBlacklist;

	return b;
}

V2I UI::calculateTextInputSize(TextInput *textInput, UITextInputStyle *style, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (style->padding * 2);
	s32 textMaxWidth = (maxWidth == 0) ? 0 : (maxWidth - doublePadding);

	BitmapFont *font = getFont(&style->font);
	String text = textInput->toString();
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

Rect2I UI::drawTextInput(RenderBuffer *renderBuffer, TextInput *textInput, UITextInputStyle *style, Rect2I bounds)
{
	DEBUG_FUNCTION();

	String text = textInput->toString();
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
