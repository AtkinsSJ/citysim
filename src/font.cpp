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

void drawText(GLRenderer *renderer, BitmapFont *font, V2 position, char *text, V4 color)
{
	for (char *currentChar=text;
		*currentChar != 0;
		currentChar++)
	{
		uint32 uChar = (uint32)(*currentChar);
		BitmapFontChar *c = findChar(font, uChar);
		if (c)
		{
			drawQuad(renderer, true,
					 rectXYWH(
					 	position.x + (real32)c->xOffset,
					 	position.y + (real32)c->yOffset,
					 	(real32)c->size.w,
					 	(real32)c->size.h
					 ),
					 0, c->textureID, c->uv, color);
			position.x += (real32)c->xAdvance;
		}
	}
}

BitmapFontCachedText *drawTextToCache(BitmapFont *font, char *text, V4 color)
{
	uint32 textLength = strlen(text);

	// Memory management witchcraft
	uint32 memorySize = sizeof(BitmapFontCachedText) + (sizeof(Sprite) * textLength);
	uint8 *data = (uint8 *) calloc(1, memorySize);
	BitmapFontCachedText *result = (BitmapFontCachedText *) data;
	result->sprites = (Sprite *)(data + sizeof(BitmapFontCachedText));

	result->size = v2(0, font->lineHeight);

	V2 position = {};

	if (result)
	{
		for (char *currentChar=text;
			*currentChar != 0;
			currentChar++)
		{
			uint32 uChar = (uint32)(*currentChar);
			BitmapFontChar *c = findChar(font, uChar);
			if (c)
			{
				*(result->sprites + result->spriteCount++) = makeSprite(
					rectXYWH(position.x + (real32)c->xOffset, position.y + (real32)c->yOffset, (real32)c->size.w, (real32)c->size.h),
					0, c->textureID, c->uv, color
				);
				position.x += (real32)c->xAdvance;
				result->size.x += (real32)c->xAdvance;
			}
		}
	}

	return result;
}

V2 calculateTextPosition(BitmapFontCachedText *cache, V2 origin, uint32 align)
{
	V2 offset;

	switch (align & ALIGN_H)
	{
		case ALIGN_H_CENTER: {
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
		case ALIGN_V_CENTER: {
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

void drawCachedText(GLRenderer *renderer, BitmapFontCachedText *cache, V2 topLeft)
{
	for (uint32 spriteIndex=0;
		spriteIndex < cache->spriteCount;
		spriteIndex++)
	{
		drawSprite(renderer, true, cache->sprites + spriteIndex, v3(topLeft.x, topLeft.y, 0));
	}
}
