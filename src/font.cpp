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

void drawText(GLRenderer *renderer, BitmapFont *font, V2 position, char *text)
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
						0, c->textureID, c->uv, 0);
			position.x += c->xAdvance;
		}
	}
}