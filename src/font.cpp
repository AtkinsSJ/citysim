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

BitmapFontGlyph *findGlyph(BitmapFont *font, unichar targetChar)
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

	ASSERT(renderBuffer != null, "RenderBuffer must be provided!");
	ASSERT(font != null, "Font must be provided!");
	
	V2 result = v2(maxWidth, (f32)font->lineHeight);

	bool doWrap = (maxWidth > 0);
	s32 currentX = 0;
	s32 currentWordWidth = 0;
	s32 whitespaceWidthBeforeCurrentWord = 0;

	s32 lineCount = 1;
	s32 currentLineWidth = 0;
	s32 longestLineWidth = 0;

	s32 bytePos = 0;
	unichar c = 0;
	bool foundNext = getNextUnichar(text, &bytePos, &c);

	while (foundNext)
	{
		if (c == '\n')
		{
			currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
			longestLineWidth = max(longestLineWidth, currentLineWidth);
			ASSERT(maxWidth < 1 || maxWidth >= longestLineWidth, "TOO BIG");

			whitespaceWidthBeforeCurrentWord = 0;
			currentWordWidth = 0;
			currentLineWidth = 0;

			currentX = 0;
			lineCount++;
		}
		else if (isWhitespace(c))
		{
			// WHITESPACE LOOP

			// If we had a previous word, we know that it must have just finished, so add the whitespace!
			currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
			whitespaceWidthBeforeCurrentWord = 0;
			currentWordWidth = 0;

			unichar prevC = 0;
			BitmapFontGlyph *glyph = null;

			do
			{
				if (c != prevC)
				{
					glyph = findGlyph(font, c);
					prevC = c;
				}

				if (glyph)
				{
					whitespaceWidthBeforeCurrentWord += glyph->xAdvance;
				}

				foundNext = getNextUnichar(text, &bytePos, &c);
			}
			while (foundNext && isWhitespace(c));

			currentX += whitespaceWidthBeforeCurrentWord;

			continue;
		}
		else
		{
			BitmapFontGlyph *glyph = findGlyph(font, c);
			if (glyph)
			{
				if (doWrap && ((currentX + glyph->xAdvance) > maxWidth))
				{
					longestLineWidth = max(longestLineWidth, currentLineWidth);

					lineCount++;

					if ((currentWordWidth + glyph->xAdvance) > maxWidth)
					{
						// The current word is longer than will fit on an entire line!
						// So, split it at the maximum line length.

						// This should mean just wrapping the final character
						currentWordWidth = 0;
						currentX = 0;

						currentLineWidth = 0;
						whitespaceWidthBeforeCurrentWord = 0;
					}
					else
					{
						// Wrapping the whole word onto a new line
						// Set the current position to where the next word will start
						currentLineWidth = 0;
						whitespaceWidthBeforeCurrentWord = 0;

						currentX = currentWordWidth;
					}
				}

				currentX += glyph->xAdvance;
				currentWordWidth += glyph->xAdvance;
			}
		}

		foundNext = getNextUnichar(text, &bytePos, &c);
	}

	result.x = max(maxWidth, (f32)max(longestLineWidth, currentX));
	result.y = (f32)(font->lineHeight * lineCount);

	ASSERT(maxWidth < 1 || maxWidth >= result.x, "Somehow we measured text that's too wide!");
	return result;
}

void _alignText(RenderItem *startOfLine, RenderItem *endOfLine, s32 lineWidth, s32 boundsWidth, u32 align)
{
	if (lineWidth == 0)
	{
		return;
	}

	switch (align & ALIGN_H)
	{
		case ALIGN_RIGHT:
		{
			s32 offsetX = boundsWidth - lineWidth;
			applyOffsetToRenderItems(startOfLine, endOfLine, (f32) offsetX, 0);
		} break;

		case ALIGN_H_CENTRE:
		{
			s32 offsetX = (boundsWidth - lineWidth) / 2;
			applyOffsetToRenderItems(startOfLine, endOfLine, (f32) offsetX, 0);
		} break;

		case ALIGN_LEFT:
		{
			// Nothing to do!
		} break;
		
		default:
		{
			DEBUG_BREAK();
		} break;
	}
}

void drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, Rect2 bounds, u32 align, f32 depth, V4 color, s32 shaderID, s32 caretPosition, DrawTextResult *caretInfoResult)
{
	DEBUG_FUNCTION();

	ASSERT(renderBuffer != null, "RenderBuffer must be provided!");
	ASSERT(font != null, "Font must be provided!");

	V2 topLeft = bounds.pos;
	s32 maxWidth = (s32) bounds.w;
	
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

	s32 currentX = 0;
	s32 currentY = 0;
	bool doWrap = (maxWidth > 0);
	s32 startOfCurrentWord = 0;
	s32 currentWordWidth = 0;

	s32 startOfCurrentLine = 0;
	s32 currentLineWidth = 0;

	s32 whitespaceWidthBeforeCurrentWord = 0;

	s32 glyphCount = 0;
	s32 bytePos = 0;
	unichar c = 0;
	bool foundNext = getNextUnichar(text, &bytePos, &c);

	while (foundNext)
	{
		if (c == '\n')
		{
			currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
			whitespaceWidthBeforeCurrentWord = 0;
			currentWordWidth = 0;
			startOfCurrentWord = 0;

			currentY += font->lineHeight;
			currentX = 0;

			// TODO: @Speed Could optimise consecutive newline characters by looping here, like with whitespace.
			// We only need to do the alignment once, for the line before the newlines!

			// Do line-alignment stuff
			{
				_alignText(firstRenderItem + startOfCurrentLine, firstRenderItem + glyphCount - 1, currentLineWidth, maxWidth, align);
				startOfCurrentLine = glyphCount;
				currentLineWidth = 0;
			}
		}
		else if (isWhitespace(c))
		{
			// WHITESPACE LOOP

			// If we had a previous word, we know that it must have just finished, so add the whitespace!
			currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
			whitespaceWidthBeforeCurrentWord = 0;
			currentWordWidth = 0;
			startOfCurrentWord = 0;

			unichar prevC = 0;
			BitmapFontGlyph *glyph = null;

			do
			{
				if (c != prevC)
				{
					glyph = findGlyph(font, c);
					prevC = c;
				}

				if (glyph)
				{
					whitespaceWidthBeforeCurrentWord += glyph->xAdvance;

					// // Using < so that in the case of caretPosition being > glyphCount, we just get the data for the last glyph!
					// if (caretInfoResult && glyphCount < caretPosition)
					// {
					// 	caretInfoResult->isValid = true;
					// 	caretInfoResult->renderItemAtPosition = firstRenderItem + glyphCount;
					// 	caretInfoResult->glyphAtPosition = glyph;
					// }

					// glyphCount++;
				}
				foundNext = getNextUnichar(text, &bytePos, &c);
			}
			while (foundNext && isWhitespace(c));

			startOfCurrentWord = glyphCount;
			currentX += whitespaceWidthBeforeCurrentWord;

			continue;
		}
		else
		{
			BitmapFontGlyph *glyph = findGlyph(font, c);
			if (glyph)
			{
				if (doWrap && ((currentX + glyph->xAdvance) > maxWidth))
				{
					if (currentWordWidth + glyph->xAdvance > maxWidth)
					{
						// The current word is longer than will fit on an entire line!
						// So, split it at the maximum line length.

						// This should mean just wrapping the final character
						currentX = 0;
						currentY += font->lineHeight;

						startOfCurrentWord = glyphCount;
						currentLineWidth = currentWordWidth;
						currentWordWidth = 0;

						RenderItem *firstItemInWord = firstRenderItem + startOfCurrentWord;
						firstItemInWord->rect.x = topLeft.x;
						firstItemInWord->rect.y = topLeft.y + currentY + glyph->yOffset;

						// Do line-alignment stuff
						{
							_alignText(firstRenderItem + startOfCurrentLine, firstRenderItem + glyphCount - 1, currentLineWidth, maxWidth, align);
							startOfCurrentLine = glyphCount;
							currentLineWidth = 0;
							whitespaceWidthBeforeCurrentWord = 0;
						}
					}
					else
					{
						// Wrap the whole word onto a new line

						// Offset from where the word was, to its new position
						f32 offsetX = topLeft.x - firstRenderItem[startOfCurrentWord].rect.x;
						f32 offsetY = (f32)font->lineHeight;
						applyOffsetToRenderItems(firstRenderItem + startOfCurrentWord, firstRenderItem + glyphCount, offsetX, offsetY);

						// Do line-alignment stuff
						{
							RenderItem *startOfLine = firstRenderItem + startOfCurrentLine;
							RenderItem *endOfLine = firstRenderItem + startOfCurrentWord - 1;

							_alignText(startOfLine, endOfLine, currentLineWidth, maxWidth, align);
							startOfCurrentLine = startOfCurrentWord;
							currentLineWidth = 0;
							whitespaceWidthBeforeCurrentWord = 0;
						}

						// Set the current position to where the next word will start
						currentX = currentWordWidth;
						currentY += font->lineHeight;
					}
				}

				makeRenderItem(firstRenderItem + glyphCount,
					rectXYWH(topLeft.x + currentX + glyph->xOffset, topLeft.y + currentY + glyph->yOffset, glyph->width, glyph->height),
					depth, font->texture, glyph->uv, shaderID, color
				);

				currentX += glyph->xAdvance;
				currentWordWidth += glyph->xAdvance;

				// // Using < so that in the case of caretPosition being > glyphCount, we just get the data for the last glyph!
				// if (caretInfoResult && glyphCount < caretPosition)
				// {
				// 	caretInfoResult->isValid = true;
				// 	caretInfoResult->renderItemAtPosition = firstRenderItem + glyphCount;
				// 	caretInfoResult->glyphAtPosition = glyph;
				// }

				glyphCount++;
			}
		}
		
		foundNext = getNextUnichar(text, &bytePos, &c);
	}


	// Align the final line
	currentLineWidth += currentWordWidth + whitespaceWidthBeforeCurrentWord;
	_alignText(firstRenderItem + startOfCurrentLine, firstRenderItem + glyphCount-1, currentLineWidth, maxWidth, align);

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
