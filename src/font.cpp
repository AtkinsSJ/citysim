// font.cpp

BitmapFontChar *findChar(BitmapFont *font, unichar targetChar)
{
	BitmapFontChar *result = 0;

	// BINARY SEARCH! :D

	// FIXME: This really needs to be replaced by a better system.

	u32 high = font->charCount;
	u32 low = 0;
	u32 current = (high + low) / 2;

	BitmapFontChar *currentChar = font->chars + current;

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

		if (high > font->charCount)
		{
			break;
		}

		current = (high + low) / 2;
		currentChar = font->chars + current;
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

	RenderItem *startOfCurrentWord;
	RenderItem *endOfCurrentWord;
	f32 currentWordWidth;

	f32 currentLineWidth;
	f32 longestLineWidth;
};

void nextLine(DrawTextState *state)
{
	state->longestLineWidth = state->maxWidth;
	state->position.y += state->lineHeight;
	state->position.x = 0;
	state->currentLineWidth = 0;
	state->lineCount++;
}

void checkAndHandleWrapping(DrawTextState *state, BitmapFontChar *c)
{
	if (state->startOfCurrentWord == null)
	{
		state->startOfCurrentWord = state->endOfCurrentWord;
		state->currentWordWidth = 0;
	}

	if (isWhitespace(c->codepoint))
	{
		state->startOfCurrentWord = null;
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

				state->startOfCurrentWord->rect.pos.x = state->position.x;
				state->startOfCurrentWord->rect.pos.y = state->position.y + (f32)c->yOffset;
			}
			else
			{
				// Wrap the whole word onto a new line
				nextLine(state);

				V2 offset = v2(-state->startOfCurrentWord->rect.x, state->lineHeight);
				while (state->startOfCurrentWord <= state->endOfCurrentWord)
				{
					state->startOfCurrentWord->rect.pos += offset;
					state->startOfCurrentWord++;
				}

				state->position.x = state->currentWordWidth;
				state->currentLineWidth = state->currentWordWidth;
			}
		}
	}

	state->position.x += c->xAdvance;
	state->currentWordWidth += c->xAdvance;
	state->currentLineWidth += c->xAdvance;
	state->longestLineWidth = MAX(state->longestLineWidth, state->currentLineWidth);
}

BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, String text,
									  V4 color, f32 maxWidth=0)
{
	if (font == null)
	{
		logError("Attempted to display text with a null font: {0}", {text});
		return null;
	}

	DrawTextState state = {};

	state.maxWidth = maxWidth;
	state.doWrap = (maxWidth > 0);
	state.lineCount = 1;
	state.longestLineWidth = 0;
	state.lineHeight = font->lineHeight;
	state.position = {};
	state.startOfCurrentWord = null;
	state.currentWordWidth = 0;
	state.currentLineWidth = 0;

	s32 glyphsToOutput = countGlyphs(text.chars, text.length);

	// Memory management witchcraft
	u32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(RenderItem) * glyphsToOutput);
	u8 *data = (u8 *) allocate(memory, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->chars = (RenderItem *)(data + sizeof(BitmapFontCachedText));
	result->size = v2(0, font->lineHeight);

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

				BitmapFontChar *c = findChar(font, glyph);
				if (c)
				{
					state.endOfCurrentWord = result->chars + result->charCount++;
					*state.endOfCurrentWord = makeRenderItem(
						rectXYWH(state.position.x + (f32)c->xOffset, state.position.y + (f32)c->yOffset,
								 (f32)c->size.w, (f32)c->size.h),
						0.0f, c->textureRegionID, color
					);

					checkAndHandleWrapping(&state, c);
				}
			}

			bytePos = findStartOfNextGlyph(text.chars, bytePos, text.length);
		}

		// Final flush to make sure the last line is correct
		// TODO: I think this isn't necessary now?
		checkAndHandleWrapping(&state, &font->nullChar);

		result->size.x = MAX(state.longestLineWidth, state.currentLineWidth);
		result->size.y = (f32)(font->lineHeight * state.lineCount);
	}

	return result;
}
BitmapFontCachedText *drawTextToCache(MemoryArena *memory, BitmapFont *font, char *text,
									  V4 color, f32 maxWidth=0)
{
	String string = stringFromChars(text);
	return drawTextToCache(memory, font, string, color, maxWidth);
}

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, u32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTRE: {
			offset.x = origin.x - round_f32(cache->size.x / 2.0f);
		} break;

		case ALIGN_RIGHT: {
			offset.x = origin.x - cache->size.x;
		} break;

		default: { // Left is default
			offset.x = origin.x;
		} break;
	}

	switch (align & ALIGN_V)
	{
		case ALIGN_V_CENTRE: {
			offset.y = origin.y - round_f32(cache->size.y / 2.0f);
		} break;

		case ALIGN_BOTTOM: {
			offset.y = origin.y - cache->size.y;
		} break;

		default: { // Top is default
			offset.y = origin.y;
		} break;
	}

	return offset;
}

void drawCachedText(RenderBuffer *uiBuffer, BitmapFontCachedText *cache, V2 topLeft, f32 depth)
{
	if (cache == null) return;

	// Make sure we're on whole-pixel boundaries for nicer text rendering
	V2 origin = v2(round_f32(topLeft.x), round_f32(topLeft.y));
	
	for (u32 spriteIndex=0;
		spriteIndex < cache->charCount;
		spriteIndex++)
	{
		drawRenderItem(uiBuffer, cache->chars + spriteIndex, origin, depth);
	}
}
