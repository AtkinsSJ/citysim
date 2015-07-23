// font.cpp

BitmapFontChar *findChar(BitmapFont *font, uint32 uChar)
{
	// TODO: Binary search!

	BitmapFontChar *result = 0;

	for (uint32 i=0;
		i < font->charCount;
		i++)
	{
		if (font->chars[i].id == uChar) {
			result = font->chars + i;
			break;
		}
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
		drawQuad(renderer, true, rectXYWH(position.x + c->xOffset, position.y + c->yOffset, c->size.w, c->size.h),
					0, c->textureID, c->uv, 0);
		position.x += c->xAdvance;
	}
}