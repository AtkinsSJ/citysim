// font.cpp

inline BitmapFontGlyphEntry *findGlyphInternal(BitmapFont *font, unichar targetChar)
{
	u32 index = targetChar % font->glyphCapacity;
	BitmapFontGlyphEntry *result = null;

	while (true)
	{
		BitmapFontGlyphEntry *entry = font->glyphEntries + index;
		if (!entry->isOccupied || entry->codepoint == targetChar)
		{
			result = entry;
			break;
		}

		index = (index + 1) % font->glyphCapacity;
	}

	return result;
}

BitmapFontGlyph *addGlyph(BitmapFont *font, unichar targetChar)
{
	BitmapFontGlyphEntry *result = findGlyphInternal(font, targetChar);

	ASSERT(result != null, "Failed to add a glyph to font '{0}'!", {font->name});
	result->codepoint = targetChar;
	ASSERT(result->isOccupied == false, "Attempted to add glyph '{0}' to font '{1}' twice!", {formatInt(targetChar), font->name});
	result->isOccupied = true;

	font->glyphCount++;

	return &result->glyph;
}

BitmapFontGlyph *findChar(BitmapFont *font, unichar targetChar)
{
	BitmapFontGlyph *result = null;
	BitmapFontGlyphEntry *entry = findGlyphInternal(font, targetChar);

	if (entry == null || !entry->isOccupied)
	{
		logWarn("Failed to find char 0x{0} in font.", {formatInt(targetChar, 16)});
	}
	else
	{
		result = &entry->glyph;
	}

	return result;
}

V2 calculateTextSize(BitmapFont *font, String text, f32 maxWidth)
{
	DEBUG_FUNCTION();
	
	V2 result = v2(0, font->lineHeight);

	if (font == null)
	{
		logError("Attempted to display text with a null font: {0}", {text});
	}
	else
	{
		s32 lineCount = 1;
		bool doWrap = (maxWidth > 0);
		s32 currentX = 0;
		s32 currentWordWidth = 0;
		s32 longestLineWidth = 0;

		s32 bytePos = 0;
		while (bytePos < text.length)
		{
			char *currentChar = text.chars + bytePos;
			unichar glyph = readUnicodeChar(currentChar);

			if (glyph == '\n')
			{
				longestLineWidth = max(longestLineWidth, currentX);
				currentX = 0;
				lineCount++;
			}
			else
			{
				BitmapFontGlyph *c = findChar(font, glyph);
				if (c)
				{
					s32 xAdvance = c->xAdvance;
					if (isWhitespace(glyph))
					{
						currentWordWidth = 0;
					}
					else if (doWrap && ((currentX + xAdvance) > maxWidth))
					{
						longestLineWidth = max(longestLineWidth, currentX);
						lineCount++;

						if ((currentWordWidth + xAdvance) > maxWidth)
						{
							// The current word is longer than will fit on an entire line!
							// So, split it at the maximum line length.

							// This should mean just wrapping the final character
							currentWordWidth = 0;
							currentX = 0;
						}
						else
						{
							// Wrapping the whole word onto a new line
							// Set the current position to where the next word will start
							currentX = currentWordWidth;
						}
					}

					currentX += xAdvance;
					currentWordWidth += xAdvance;
				}
			}

			bytePos += lengthOfGlyph(*currentChar);
		}

		result.x = (f32)max(longestLineWidth, currentX);
		result.y = (f32)(font->lineHeight * lineCount);
	}

	return result;
}

void drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2 topLeft, f32 maxWidth, f32 depth, V4 color, s32 shaderID, s32 caretPosition, DrawTextResult *caretInfoResult)
{
	DEBUG_FUNCTION();

	//
	// NB: We *could* just use text.length here. That will over-estimate how many RenderItems to reserve,
	// which is fine if we're sticking with mostly English text, but in languages with a lot of multi-byte
	// characters, could involve reserving 2 or 3 times what we actually need.
	// countGlyphs() is not very fast, though it's less bad than I thought once I take the profiling out
	// of it. It accounts for about 4% of the time for drawText(), which is 0.1ms in my stress-test.
	// Also, the over-estimate will only boost the size of the renderitems array *once* as it never shrinks.
	// We could end up with one that's, I dunno, twice the size it needs to be... RenderItem is 64 bytes
	// right now, so 20,000 of them is around 1.25MB, which isn't a big deal.
	//
	// I think I'm going to go with the faster option for now, and maybe revisit this in the future.
	// Eventually we'll switch to unichar-strings, so all of this will be irrelevant anyway!
	//
	// - Sam, 28/06/2019
	//
	// s32 glyphsToOutput = countGlyphs(text.chars, text.length);
	s32 glyphsToOutput = text.length;

	RenderItem *firstRenderItem = reserveRenderItemRange(renderBuffer, glyphsToOutput);

	f32 currentX = 0, currentY = 0;
	bool doWrap = (maxWidth > 0);
	s32 startOfCurrentWord = 0;
	s32 endOfCurrentWord = 0;
	s32 currentWordWidth = 0;

	s32 glyphCount = 0; // Not the same as glyphIndex, because some glyphs are non-printing!
	s32 bytePos = 0;
	while (bytePos < text.length)
	{
		unichar glyph = readUnicodeChar(text.chars + bytePos);

		if (glyph == '\n')
		{
			currentY += font->lineHeight;
			currentX = 0;
		}
		else
		{
			BitmapFontGlyph *c = findChar(font, glyph);
			if (c)
			{
				endOfCurrentWord = glyphCount;
				makeRenderItem(firstRenderItem + glyphCount,
					rectXYWH(topLeft.x + currentX + c->xOffset, topLeft.y + currentY + c->yOffset, c->width, c->height),
					depth, font->pageTextures[c->page], c->uv, shaderID, color
				);

				if (startOfCurrentWord == 0)
				{
					startOfCurrentWord = endOfCurrentWord;
					currentWordWidth = 0;
				}

				if (isWhitespace(glyph))
				{
					startOfCurrentWord = 0;
					currentWordWidth = 0;
				}
				else if (doWrap && ((currentX + c->xAdvance) > maxWidth))
				{
					if (currentWordWidth + c->xAdvance > maxWidth)
					{
						// The current word is longer than will fit on an entire line!
						// So, split it at the maximum line length.

						// This should mean just wrapping the final character
						currentX = 0;
						currentY += font->lineHeight;

						startOfCurrentWord = endOfCurrentWord;
						currentWordWidth = 0;

						RenderItem *firstItemInWord = firstRenderItem + startOfCurrentWord;
						firstItemInWord->rect.x = topLeft.x + currentX;
						firstItemInWord->rect.y = topLeft.y + currentY + c->yOffset;
					}
					else
					{
						// Wrap the whole word onto a new line

						// Set the current position to where the next word will start
						currentX = (f32) currentWordWidth;
						currentY += font->lineHeight;

						// Offset from where the word was, to its new position
						V2 offset = v2(topLeft.x - firstRenderItem[startOfCurrentWord].rect.x, (f32)font->lineHeight);
						while (startOfCurrentWord <= endOfCurrentWord)
						{
							firstRenderItem[startOfCurrentWord].rect.pos += offset;
							startOfCurrentWord++;
						}
					}
				}

				currentX += c->xAdvance;
				currentWordWidth += c->xAdvance;

				// Using < so that in the case of caretPosition being > glyphCount, we just get the data for the last glyph!
				if (caretInfoResult && glyphCount < caretPosition)
				{
					caretInfoResult->isValid = true;
					caretInfoResult->renderItemAtPosition = firstRenderItem + glyphCount;
					caretInfoResult->glyphAtPosition = c;
				}

				glyphCount++;
			}
		}

		bytePos += lengthOfGlyph(text.chars[bytePos]);
	}

	finishReservedRenderItemRange(renderBuffer, glyphCount);
}

V2 calculateTextPosition(V2 origin, V2 size, u32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  offset.x = origin.x - (size.x / 2.0f);  break;
		case ALIGN_RIGHT:     offset.x = origin.x - size.x;           break;
		case ALIGN_LEFT:      // Left is default
		default:              offset.x = origin.x;                    break;
	}

	switch (align & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  offset.y = origin.y - (size.y / 2.0f);  break;
		case ALIGN_BOTTOM:    offset.y = origin.y - size.y;           break;
		case ALIGN_TOP:       // Top is default
		default:              offset.y = origin.y;                    break;
	}

	offset.x = round_f32(offset.x);
	offset.y = round_f32(offset.y);

	return offset;
}
