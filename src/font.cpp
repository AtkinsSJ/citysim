// font.cpp

BitmapFontChar *findChar(BitmapFont *font, unichar targetChar)
{
	BitmapFontChar *result = 0;

	// BINARY SEARCH! :D

	// FIXME: This really needs to be replaced by a better system.

	uint32 high = font->charCount;
	uint32 low = 0;
	uint32 current = (high + low) / 2;

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
		SDL_Log("Failed to find char 0x%X in font.", targetChar);
	}

	return result;
}

struct DrawTextState
{
	bool doWrap;
	real32 maxWidth;
	real32 lineHeight;
	int32 lineCount;

	V2 position;

	RenderItem *startOfCurrentWord;
	RenderItem *endOfCurrentWord;
	real32 currentWordWidth;

	real32 currentLineWidth;
	real32 longestLineWidth;
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
			real32 newWordWidth = state->currentWordWidth + c->xAdvance;
			if (newWordWidth > state->maxWidth)
			{
				// The current word is longer than will fit on an entire line!
				// So, split it at the maximum line length.

				// This should mean just wrapping the final character
				nextLine(state);

				state->startOfCurrentWord = state->endOfCurrentWord;
				state->currentWordWidth = 0;

				state->startOfCurrentWord->rect.pos.x = state->position.x;
				state->startOfCurrentWord->rect.pos.y = state->position.y + (real32)c->yOffset;
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

BitmapFontCachedText *drawTextToCache(TemporaryMemory *memory, BitmapFont *font, String text,
									  V4 color, real32 maxWidth=0)
{
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

	int32 glyphsToOutput = countGlyphs(text.chars, text.length);

	// Memory management witchcraft
	uint32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(RenderItem) * glyphsToOutput);
	uint8 *data = (uint8 *) allocate(memory, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->chars = (RenderItem *)(data + sizeof(BitmapFontCachedText));
	result->size = v2(0, font->lineHeight);

	if (result)
	{
		int32 bytePos = 0;
		for (int32 glyphIndex = 0; glyphIndex < glyphsToOutput; glyphIndex++)
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
						rectXYWH(state.position.x + (real32)c->xOffset, state.position.y + (real32)c->yOffset,
								 (real32)c->size.w, (real32)c->size.h),
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
		result->size.y = (real32)(font->lineHeight * state.lineCount);
	}

	return result;
}
BitmapFontCachedText *drawTextToCache(TemporaryMemory *memory, BitmapFont *font, char *text,
									  V4 color, real32 maxWidth=0)
{
	String string = stringFromChars(text);
	return drawTextToCache(memory, font, string, color, maxWidth);
}

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, uint32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTRE: {
			offset.x = origin.x - cache->size.x / 2.0f;
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
			offset.y = origin.y - cache->size.y / 2.0f;
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

void drawCachedText(RenderBuffer *uiBuffer, BitmapFontCachedText *cache, V2 topLeft, real32 depth)
{
	for (uint32 spriteIndex=0;
		spriteIndex < cache->charCount;
		spriteIndex++)
	{
		drawRenderItem(uiBuffer, cache->chars + spriteIndex, topLeft, depth);
	}
}
