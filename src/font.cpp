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

DrawTextState makeDrawTextState(f32 maxWidth, f32 lineHeight, RenderItem *firstRenderItem)
{
	DrawTextState result = {};

	result.doWrap = (maxWidth > 0);
	result.maxWidth = maxWidth;
	result.lineHeight = lineHeight;
	result.lineCount = 1;

	result.firstRenderItem = firstRenderItem;

	return result;
}

inline void nextLine(DrawTextState *state)
{
	state->longestLineWidth = state->maxWidth;
	state->position.y += state->lineHeight;
	state->position.x = 0;
	state->currentLineWidth = 0;
	state->lineCount++;
}

void handleWrapping(DrawTextState *state, BitmapFontGlyph *c)
{
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
	else if (state->doWrap)
	{
		// check possible reasons for wrapping.
		// actually, that's always because we're too wide

		if ((state->currentLineWidth + c->xAdvance) > state->maxWidth)
		{
			f32 newWordWidth = state->currentWordWidth + c->xAdvance;
			if (newWordWidth > state->maxWidth)
			{
				// The current word is longer than will fit on an entire line!
				// So, split it at the maximum line length.

				// This should mean just wrapping the final character
				nextLine(state);

				state->startOfCurrentWord = state->endOfCurrentWord;
				state->currentWordWidth = 0;

				if (state->firstRenderItem)
				{
					RenderItem *firstItemInWord = state->firstRenderItem + state->startOfCurrentWord;
					firstItemInWord->rect.pos.x = state->position.x;
					firstItemInWord->rect.pos.y = state->position.y + (f32)c->yOffset;
				}
			}
			else
			{
				// Wrap the whole word onto a new line
				nextLine(state);

				state->position.x = state->currentWordWidth;
				state->currentLineWidth = state->currentWordWidth;

				if (state->firstRenderItem)
				{
					V2 offset = v2(-state->firstRenderItem[state->startOfCurrentWord].rect.x, state->lineHeight);
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
	}

	state->position.x       += c->xAdvance;
	state->currentWordWidth += c->xAdvance;
	state->currentLineWidth += c->xAdvance;
	state->longestLineWidth = max(state->longestLineWidth, state->currentLineWidth);
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
	DrawTextState state = makeDrawTextState(maxWidth, font->lineHeight);

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	s32 bytePos = 0;
	for (s32 glyphIndex = 0; glyphIndex < glyphsToOutput; glyphIndex++)
	{
		unichar glyph = readUnicodeChar(text.chars + bytePos);

		if (glyph == '\n')
		{
			nextLine(&state);
		}
		else
		{
			BitmapFontGlyph *c = findChar(font, glyph);
			if (c)
			{
				// NB: the actual VALUE of endOfCurrentWord doesn't matter here, it just needs to be
				// *something* that isn't the default -1 value.
				// This whole thing is hacks on top of hacks, and I really don't like it.
				state.endOfCurrentWord = glyphIndex;
				handleWrapping(&state, c);
			}
		}

		bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
	}

	result.x = max(state.longestLineWidth, state.currentLineWidth);
	result.y = (f32)(font->lineHeight * state.lineCount);

	return result;
}

BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, String text, f32 maxWidth)
{
	DEBUG_FUNCTION();
	
	if (font == null)
	{
		logError("Attempted to display text with a null font: {0}", {text});
		return null;
	}

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	// Memory management witchcraft
	smm baseStructSize    = sizeof(BitmapFontCachedText);
	smm renderItemsSize   = sizeof(RenderItem) * glyphsToOutput;
	smm glyphPointersSize = sizeof(BitmapFontGlyph*) * glyphsToOutput;

	u8 *data = (u8 *) allocate(memory, baseStructSize + renderItemsSize + glyphPointersSize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->renderItems = (RenderItem *)(data + baseStructSize);
	result->glyphs = (BitmapFontGlyph **)(data + baseStructSize + renderItemsSize);
	result->bounds = v2(0, font->lineHeight);

	DrawTextState state = makeDrawTextState(maxWidth, font->lineHeight, result->renderItems);

	if (result)
	{
		s32 bytePos = 0;
		for (s32 glyphIndex = 0; glyphIndex < glyphsToOutput; glyphIndex++)
		{
			unichar glyph = readUnicodeChar(text.chars + bytePos);

			if (glyph == '\n')
			{
				nextLine(&state);
			}
			else
			{
				BitmapFontGlyph *c = findChar(font, glyph);
				if (c)
				{
					state.endOfCurrentWord = result->glyphCount;
					makeRenderItem(result->renderItems + result->glyphCount, 
						rectXYWH(state.position.x + (f32)c->xOffset, state.position.y + (f32)c->yOffset,
								 (f32)c->size.w, (f32)c->size.h),
						0.0f, font->pageTextures[c->page], c->uv, -1
					);

					result->glyphs[result->glyphCount] = c;

					result->glyphCount++;

					handleWrapping(&state, c);
				}
			}

			bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
		}

		result->bounds.x = max(state.longestLineWidth, state.currentLineWidth);
		result->bounds.y = (f32)(font->lineHeight * state.lineCount);
	}

	return result;
}

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, u32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  offset.x = origin.x - (cache->bounds.x / 2.0f);  break;
		case ALIGN_RIGHT:     offset.x = origin.x - cache->bounds.x;           break;
		case ALIGN_LEFT:      // Left is default
		default:              offset.x = origin.x;                           break;
	}

	switch (align & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  offset.y = origin.y - (cache->bounds.y / 2.0f);  break;
		case ALIGN_BOTTOM:    offset.y = origin.y - cache->bounds.y;           break;
		case ALIGN_TOP:       // Top is default
		default:              offset.y = origin.y;                           break;
	}

	offset.x = round_f32(offset.x);
	offset.y = round_f32(offset.y);

	return offset;
}

void drawCachedText(RenderBuffer *uiBuffer, BitmapFontCachedText *cache, V2 topLeft, f32 depth, V4 color, s32 shaderID)
{
	DEBUG_FUNCTION();
	
	if (cache == null) return;

	// NB: No need to round topLeft to a whole number position, because we always get it from calculateTextPosition()
	// which does the work for us!
	
	for (u32 renderItemIndex=0;
		renderItemIndex < cache->glyphCount;
		renderItemIndex++)
	{
		drawRenderItem(uiBuffer, cache->renderItems + renderItemIndex, topLeft, depth, color, shaderID);
	}
}
