// font.cpp

BitmapFontChar *findChar(BitmapFont *font, uint32 targetChar)
{
	BitmapFontChar *result = 0;

	// BINARY SEARCH! :D

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

void drawText(GLRenderer *renderer, BitmapFont *font, V2 position, char *text, Color *color)
{
	for (char *currentChar=text;
		*currentChar != 0;
		currentChar++)
	{
		uint32 uChar = (uint32)(*currentChar);
		BitmapFontChar *c = findChar(font, uChar);
		if (c)
		{
			drawQuad(renderer, true, rectXYWH(position.x + c->xOffset, position.y + c->yOffset, c->size.w, c->size.h),
						0, c->textureID, c->uv, color);
			position.x += c->xAdvance;
		}
	}
}

BitmapFontCachedText *drawTextToCache(GLRenderer *renderer, BitmapFont *font, V2 position, char *text, Color *color)
{
	uint32 textLength = strlen(text);

	// Memory management witchcraft
	uint32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(Sprite) * textLength);
	uint8 *data = (uint8 *) calloc(1, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->sprites = (Sprite *)(data + sizeof(BitmapFontCachedText));

	if (result)
	{
		V4 drawColor;
		if (color)
		{
			drawColor = v4(color);
		} else {
			drawColor = v4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		for (char *currentChar=text;
			*currentChar != 0;
			currentChar++)
		{
			uint32 uChar = (uint32)(*currentChar);
			BitmapFontChar *c = findChar(font, uChar);
			if (c)
			{
				*(result->sprites + result->spriteCount++) = {
					rectXYWH(position.x + c->xOffset, position.y + c->yOffset, c->size.w, c->size.h),
					0, c->textureID, c->uv, drawColor
				};
				position.x += c->xAdvance;
			}
		}
	}

	return result;
}

void drawCachedText(GLRenderer *renderer, BitmapFontCachedText *cache, V2 position, uint32 align)
{
	IMPLEMENT THIS NEXT!
	Note that I haven`t had time to test/check drawTextToCache() above, or that it`s even used anywhere!
	Actually, I never set the cache`s .size, which is important!

	// for (char *currentChar=text;
	// 	*currentChar != 0;
	// 	currentChar++)
	// {
	// 	uint32 uChar = (uint32)(*currentChar);
	// 	BitmapFontChar *c = findChar(font, uChar);
	// 	if (c)
	// 	{
	// 		drawQuad(renderer, true, rectXYWH(position.x + c->xOffset, position.y + c->yOffset, c->size.w, c->size.h),
	// 					0, c->textureID, c->uv, color);
	// 		position.x += c->xAdvance;
	// 	}
	// }
}
