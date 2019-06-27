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
	DEBUG_FUNCTION();

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

inline void goToNewLine(DrawTextState *state)
{
	state->longestLineWidth = max(state->longestLineWidth, state->currentPositionRelative.x);
	state->currentPositionRelative.y += state->lineHeight;
	state->currentPositionRelative.x = 0;
	state->lineCount++;
}

void handleWrapping(DrawTextState *state, BitmapFontGlyph *c)
{
	DEBUG_FUNCTION();
	
	if (state->startOfCurrentWord == 0)
	{
		state->startOfCurrentWord = state->endOfCurrentWord;
		state->currentWordWidth = 0;
	}

	if (isWhitespace(c->codepoint))
	{
		state->startOfCurrentWord = 0;
		state->currentWordWidth = 0;
	}
	else if (state->doWrap && ((state->currentPositionRelative.x + c->xAdvance) > state->maxWidth))
	{
		if (state->currentWordWidth + c->xAdvance > state->maxWidth)
		{
			// The current word is longer than will fit on an entire line!
			// So, split it at the maximum line length.

			// This should mean just wrapping the final character
			goToNewLine(state);

			state->startOfCurrentWord = state->endOfCurrentWord;
			state->currentWordWidth = 0;

			// Move render items if we have them
			if (state->firstRenderItem)
			{
				RenderItem *firstItemInWord = state->firstRenderItem + state->startOfCurrentWord;
				firstItemInWord->rect.pos = state->origin + state->currentPositionRelative + v2(0, c->yOffset);
			}
		}
		else
		{
			// Wrap the whole word onto a new line
			goToNewLine(state);

			// Set the current position to where the next word will start
			state->currentPositionRelative.x = state->currentWordWidth;

			if (state->firstRenderItem)
			{
				// Offset from where the word was, to its new position
				V2 offset = v2(state->origin.x - state->firstRenderItem[state->startOfCurrentWord].rect.x, state->lineHeight);
				while (state->startOfCurrentWord <= state->endOfCurrentWord)
				{
					state->firstRenderItem[state->startOfCurrentWord].rect.pos += offset;
					state->startOfCurrentWord++;
				}
			}
			else
			{
				state->startOfCurrentWord = state->endOfCurrentWord;
			}
		}
	}

	state->currentPositionRelative.x += c->xAdvance;
	state->currentWordWidth += c->xAdvance;
	state->longestLineWidth = max(state->longestLineWidth, state->currentPositionRelative.x);
}

V2 calculateTextSize(BitmapFont *font, String text, f32 maxWidth)
{
	DEBUG_FUNCTION();
	
	V2 result = v2(0, font->lineHeight);

	if (font == null)
	{
		logError("Attempted to display text with a null font: {0}", {text});
		return result;
	}

	// COPIED from drawTextToCache() - maybe we want these to both be the same code path?
	DrawTextState state(maxWidth, font->lineHeight);

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	s32 bytePos = 0;
	for (s32 glyphIndex = 0; glyphIndex < glyphsToOutput; glyphIndex++)
	{
		unichar glyph = readUnicodeChar(text.chars + bytePos);

		if (glyph == '\n')
		{
			goToNewLine(&state);
		}
		else
		{
			BitmapFontGlyph *c = findChar(font, glyph);
			if (c)
			{
				// NB: the actual VALUE of endOfCurrentWord doesn't matter here, it just needs to be
				// *something* that isn't the default 0 value.
				// This whole thing is hacks on top of hacks, and I really don't like it.
				state.endOfCurrentWord = glyphIndex;

				if (state.startOfCurrentWord == 0)
				{
					state.startOfCurrentWord = state.endOfCurrentWord;
					state.currentWordWidth = 0;
				}

				if (isWhitespace(c->codepoint))
				{
					state.startOfCurrentWord = 0;
					state.currentWordWidth = 0;
				}
				else if (state.doWrap && ((state.currentPositionRelative.x + c->xAdvance) > state.maxWidth))
				{
					goToNewLine(&state);
					state.startOfCurrentWord = state.endOfCurrentWord;

					if ((state.currentWordWidth + c->xAdvance) > state.maxWidth)
					{
						// The current word is longer than will fit on an entire line!
						// So, split it at the maximum line length.

						// This should mean just wrapping the final character
						state.currentWordWidth = 0;
					}
					else
					{
						// Wrapping the whole word onto a new line
						// Set the current position to where the next word will start
						state.currentPositionRelative.x = state.currentWordWidth;
					}
				}

				state.currentPositionRelative.x += c->xAdvance;
				state.currentWordWidth += c->xAdvance;
			}
		}

		bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
	}

	result.x = max(state.longestLineWidth, state.currentPositionRelative.x);
	result.y = (f32)(font->lineHeight * state.lineCount);

	return result;
}

void drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2 topLeft, f32 maxWidth, f32 depth, V4 color, s32 shaderID, s32 caretPosition, DrawTextResult *caretInfoResult)
{
	DEBUG_FUNCTION();

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	RenderItem *firstRenderItem = reserveRenderItemRange(renderBuffer, glyphsToOutput);

	DrawTextState state(maxWidth, font->lineHeight, topLeft, firstRenderItem);

	s32 glyphCount = 0; // Not the same as glyphIndex, because some glyphs are non-printing!
	s32 bytePos = 0;
	for (s32 glyphIndex = 0; glyphIndex < glyphsToOutput; glyphIndex++)
	{
		unichar glyph = readUnicodeChar(text.chars + bytePos);

		if (glyph == '\n')
		{
			goToNewLine(&state);
		}
		else
		{
			BitmapFontGlyph *c = findChar(font, glyph);
			if (c)
			{
				state.endOfCurrentWord = glyphCount;
				makeRenderItem(firstRenderItem + glyphCount,
					rectPosSize(topLeft + state.currentPositionRelative + v2(c->xOffset, c->yOffset), v2(c->width, c->height)),
					depth, font->pageTextures[c->page], c->uv, shaderID, color
				);

				handleWrapping(&state, c);

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

		bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
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
