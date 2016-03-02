// font.cpp

BitmapFontChar *findChar(BitmapFont *font, uint32 targetChar)
{
	BitmapFontChar *result = 0;

	// BINARY SEARCH! :D

	// FIXME: This really needs to be replaced by a better system. And UTF8 support.

	uint32 high = font->charCount;
	uint32 low = 0;
	uint32 current = (high + low) / 2;

	BitmapFontChar *currentChar = font->chars + current;

	while (high >= low)
	{
		if (currentChar->id == targetChar)
		{
			result = currentChar;
			break;
		}

		if (currentChar->id < targetChar)
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

bool isWhitespace(uint32 uChar)
{
	// TODO: FINISH THIS!

	bool result = false;

	switch (uChar)
	{
	case 0:
	case 32:
		result = true;
		break;

	default:
		result = false;
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

	Sprite *startOfCurrentWord;
	Sprite *endOfCurrentWord;
	real32 currentWordWidth;

	real32 currentLineWidth;
	real32 longestLineWidth;
};

void font_newLine(DrawTextState *state)
{
	state->longestLineWidth = state->maxWidth;
	state->position.y += state->lineHeight;
	state->position.x = 0;
	state->currentLineWidth = 0;
	state->lineCount++;
}

void font_handleEndOfWord(DrawTextState *state, BitmapFontChar *c)
{
	if (state->doWrap)
	{
		if (isWhitespace(c->id))
		{
			if (state->startOfCurrentWord)
			{
				if ((state->currentWordWidth + state->currentLineWidth) > state->maxWidth)
				{
					// Wrap it
					V2 offset = v2(-state->startOfCurrentWord->rect.x, state->lineHeight);
					state->position.y += state->lineHeight;
					state->position.x = state->currentWordWidth;
					state->currentLineWidth = state->currentWordWidth;
					state->lineCount++;

					// Move all the sprites to their new positions
					while (state->startOfCurrentWord <= state->endOfCurrentWord)
					{
						state->startOfCurrentWord->rect.pos += offset;
						state->startOfCurrentWord++;
					}

					if (state->currentWordWidth > state->maxWidth)
					{
						// TODO: Split the word across multiple lines?
						// For now, we just have it overflow, and move onto another new line
						font_newLine(state);
					}
				}
				else
				{
					state->currentLineWidth += state->currentWordWidth;
					state->longestLineWidth = max(state->longestLineWidth, state->currentLineWidth);
				}

				state->startOfCurrentWord = null;
			}
			state->currentLineWidth += c->xAdvance;
			state->position.x += c->xAdvance;
		}
		else
		{
			if (state->startOfCurrentWord == null)
			{
				state->startOfCurrentWord = state->endOfCurrentWord;
				state->currentWordWidth = 0;
			}
			state->position.x += (real32)c->xAdvance;
			state->currentWordWidth += c->xAdvance;
		}
	}
	else
	{
		state->position.x += (real32)c->xAdvance;
		state->longestLineWidth = max(state->longestLineWidth, state->position.x);
	}
}

BitmapFontCachedText *drawTextToCache(TemporaryMemoryArena *memory, BitmapFont *font, char *text,
									  V4 color, real32 maxWidth=0)
{
	DrawTextState state = {};

	state.maxWidth = maxWidth;
	state.doWrap = (maxWidth > 0);
	state.lineCount = 1;
	state.longestLineWidth = 0;
	state.lineHeight = font->lineHeight;
	state.position = {};

	// Memory management witchcraft
	uint32 textLength = strlen(text);
	uint32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(Sprite) * textLength);
	uint8 *data = (uint8 *) allocate(memory, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->sprites = (Sprite *)(data + sizeof(BitmapFontCachedText));
	result->size = v2(0, font->lineHeight);

	if (result)
	{
		state.startOfCurrentWord = null;
		state.currentWordWidth = 0;
		state.currentLineWidth = 0;

		for (char *currentChar=text;
			*currentChar != 0;
			currentChar++)
		{
			if (*currentChar == '\n')
			{
				font_newLine(&state);
			}
			else
			{
				BitmapFontChar *c = findChar(font, (uint32)(*currentChar));
				if (c)
				{
					state.endOfCurrentWord = result->sprites + result->spriteCount++;
					*state.endOfCurrentWord = makeSprite(
						rectXYWH(state.position.x + (real32)c->xOffset, state.position.y + (real32)c->yOffset,
								 (real32)c->size.w, (real32)c->size.h),
						0, c->textureID, c->uv, color
					);

					font_handleEndOfWord(&state, c);
				}
			}
		}
		// Final flush to make sure the last line is correct
		font_handleEndOfWord(&state, &font->nullChar);

		result->size.x = max(state.longestLineWidth, state.currentLineWidth);
		result->size.y = (real32)(font->lineHeight * state.lineCount);
	}

	return result;
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

void drawCachedText(GLRenderer *renderer, BitmapFontCachedText *cache, V2 topLeft, real32 depth)
{
	for (uint32 spriteIndex=0;
		spriteIndex < cache->spriteCount;
		spriteIndex++)
	{
		drawSprite(renderer, true, cache->sprites + spriteIndex, v3(topLeft.x, topLeft.y, depth));
	}
}
