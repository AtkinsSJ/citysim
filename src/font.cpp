// font.cpp

BitmapFontGlyph *findChar(BitmapFont *font, unichar targetChar)
{
	BitmapFontGlyph *result = 0;

	// BINARY SEARCH! :D

	// FIXME: This really needs to be replaced by a better system.

	u32 high = font->glyphCount;
	u32 low = 0;
	u32 current = (high + low) / 2;

	BitmapFontGlyph *currentChar = font->glyphs + current;

	while (high >= low)
	{
		if (currentChar->codepoint == targetChar)
		{
			result = currentChar;
			break;
		}

		if (currentChar->codepoint < targetChar)
		{
			low = current + 1;
		}
		else
		{
			high = current - 1;
		}

		if (high > font->glyphCount)
		{
			break;
		}

		current = (high + low) / 2;
		currentChar = font->glyphs + current;
	}

	if (!result)
	{
		log("Failed to find char 0x{0} in font.", {formatInt(targetChar, 16)});
	}

	return result;
}

struct DrawTextState
{
	bool doWrap;
	f32 maxWidth;
	f32 lineHeight;
	s32 lineCount;

	V2 position;

	RenderItem *firstRenderItem;
	s32 startOfCurrentWord;
	s32 endOfCurrentWord;
	f32 currentWordWidth;

	f32 currentLineWidth;
	f32 longestLineWidth;
};

static DrawTextState makeDrawTextState(f32 maxWidth, f32 lineHeight, RenderItem *firstRenderItem=null)
{
	DrawTextState result = {};

	result.doWrap = (maxWidth > 0);
	result.maxWidth = maxWidth;
	result.lineHeight = lineHeight;
	result.lineCount = 1;

	result.firstRenderItem = firstRenderItem;

	return result;
}

static void nextLine(DrawTextState *state)
{
	state->longestLineWidth = state->maxWidth;
	state->position.y += state->lineHeight;
	state->position.x = 0;
	state->currentLineWidth = 0;
	state->lineCount++;
}

static void handleWrapping(DrawTextState *state, BitmapFontGlyph *c)
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
	state->longestLineWidth = MAX(state->longestLineWidth, state->currentLineWidth);
}

V2 calculateTextSize(BitmapFont *font, String text, f32 maxWidth=0)
{
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

	result.x = MAX(state.longestLineWidth, state.currentLineWidth);
	result.y = (f32)(font->lineHeight * state.lineCount);

	return result;
}

BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, String text, f32 maxWidth=0)
{
	if (font == null)
	{
		logError("Attempted to display text with a null font: {0}", {text});
		return null;
	}

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	// Memory management witchcraft
	u32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(RenderItem) * glyphsToOutput);
	u8 *data = (u8 *) allocate(memory, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->chars = (RenderItem *)(data + sizeof(BitmapFontCachedText));
	result->size = v2(0, font->lineHeight);

	DrawTextState state = makeDrawTextState(maxWidth, font->lineHeight, result->chars);

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
					state.endOfCurrentWord = result->charCount;
					result->chars[result->charCount] = makeRenderItem(
						rectXYWH(state.position.x + (f32)c->xOffset, state.position.y + (f32)c->yOffset,
								 (f32)c->size.w, (f32)c->size.h),
						0.0f, c->spriteID
					);

					result->charCount++;

					handleWrapping(&state, c);
				}
			}

			bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
		}

		result->size.x = MAX(state.longestLineWidth, state.currentLineWidth);
		result->size.y = (f32)(font->lineHeight * state.lineCount);

		#if BUILD_DEBUG
			V2 verificationSize = calculateTextSize(font, text, maxWidth);
			ASSERT(equals(verificationSize.x, result->size.x, 0.01f)
				&& equals(verificationSize.y, result->size.y, 0.01f), "calculateTextSize() is wrong!");
		#endif
	}

	return result;
}

BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, char *text, f32 maxWidth=0)
{
	String string = makeString(text);
	return drawTextToCache(memory, font, string, maxWidth);
}

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, u32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  offset.x = origin.x - (cache->size.x / 2.0f);  break;
		case ALIGN_RIGHT:     offset.x = origin.x - cache->size.x;           break;
		case ALIGN_LEFT:      // Left is default
		default:              offset.x = origin.x;                           break;
	}

	switch (align & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  offset.y = origin.y - (cache->size.y / 2.0f);  break;
		case ALIGN_BOTTOM:    offset.y = origin.y - cache->size.y;           break;
		case ALIGN_TOP:       // Top is default
		default:              offset.y = origin.y;                           break;
	}

	offset.x = round_f32(offset.x);
	offset.y = round_f32(offset.y);

	return offset;
}

void drawCachedText(RenderBuffer *uiBuffer, BitmapFontCachedText *cache, V2 topLeft, f32 depth, V4 color)
{
	if (cache == null) return;

	// Make sure we're on whole-pixel boundaries for nicer text rendering
	V2 origin = v2(round_f32(topLeft.x), round_f32(topLeft.y));
	
	for (u32 spriteIndex=0;
		spriteIndex < cache->charCount;
		spriteIndex++)
	{
		drawRenderItem(uiBuffer, cache->chars + spriteIndex, origin, depth, color);
	}
}
