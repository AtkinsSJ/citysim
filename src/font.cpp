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
	return (uChar == 32);
}

BitmapFontCachedText *drawTextToCache(TemporaryMemoryArena *memory, BitmapFont *font, char *text,
									  V4 color, real32 maxWidth=0)
{
	uint32 textLength = strlen(text);
	bool doWrap = (maxWidth > 0);
	int32 lineCount = 1;
	real32 longestLineWidth = 0;

	// Memory management witchcraft
	uint32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(Sprite) * textLength);
	uint8 *data = (uint8 *) allocate(memory, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->sprites = (Sprite *)(data + sizeof(BitmapFontCachedText));
	result->size = v2(0, font->lineHeight);

	V2 position = {};

	if (result)
	{
		Sprite *startOfCurrentWord = null;
		real32 currentWordWidth = 0;
		real32 currentLineWidth = 0;

		for (char *currentChar=text;
			*currentChar != 0;
			currentChar++)
		{
			uint32 uChar = (uint32)(*currentChar);
			BitmapFontChar *c = findChar(font, uChar);
			if (c)
			{
				Sprite *sprite = result->sprites + result->spriteCount++;
				*sprite = makeSprite(
					rectXYWH(position.x + (real32)c->xOffset, position.y + (real32)c->yOffset,
							 (real32)c->size.w, (real32)c->size.h),
					0, c->textureID, c->uv, color
				);

				if (doWrap)
				{
					if (isWhitespace(uChar))
					{
						if (startOfCurrentWord)
						{
							// If our current word is too long for the gap, wrap it
							if (currentWordWidth > maxWidth)
							{
								// Split it
								ASSERT(false, "Not implemented yet!");

							}
							else if ((currentWordWidth + currentLineWidth) > maxWidth)
							{
								// Wrap it
								V2 offset = v2(-startOfCurrentWord->rect.x, (real32)font->lineHeight);
								position.y += font->lineHeight;
								position.x = currentWordWidth;
								currentLineWidth = 0;
								lineCount++;

								// Move all the sprites to their new positions
								while (startOfCurrentWord != sprite)
								{
									startOfCurrentWord->rect.pos += offset;
									startOfCurrentWord++;
								}
							}
							else
							{
								currentLineWidth += currentWordWidth;
								longestLineWidth = max(longestLineWidth, currentLineWidth);
							}

							startOfCurrentWord = null;
						}
						currentLineWidth += c->xAdvance;
						position.x += c->xAdvance;
					}
					else
					{
						if (startOfCurrentWord == null)
						{
							startOfCurrentWord = sprite;
							currentWordWidth = 0;
						}
						position.x += (real32)c->xAdvance;
						currentWordWidth += c->xAdvance;
					}
				}
				else
				{
					position.x += (real32)c->xAdvance;
					longestLineWidth = max(longestLineWidth, position.x);
				}
			}
		}

		result->size.x = max(longestLineWidth, currentLineWidth);
		result->size.y = (real32)(font->lineHeight * lineCount);
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
