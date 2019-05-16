#pragma once

void loadBMFont(AssetManager *assets, Blob data, Asset *asset)
{
	smm pos = 0;
	BMFontHeader *header = (BMFontHeader *)(data.memory + pos);
	pos += sizeof(BMFontHeader);

	// Check it's a valid BMF
	if (header->tag[0] != BMFontTag[0]
	 || header->tag[1] != BMFontTag[1]
	 || header->tag[2] != BMFontTag[2])
	{
		logError("Not a valid BMFont file: {0}", {asset->fullName});
	}
	else if (header->version != 3)
	{
		logError("BMFont file version is unsupported: {0}, wanted {1} and got {2}",
						{asset->fullName, formatInt(BMFontSupportedVersion), formatInt(header->version)});
	}
	else
	{
		BMFontBlockHeader *blockHeader = 0;
		BMFontBlock_Common *common = 0;
		BMFont_Char *chars = 0;
		u32 charCount = 0;
		void *pages = 0;
			
		blockHeader = (BMFontBlockHeader *)(data.memory + pos);
		pos += sizeof(BMFontBlockHeader);

		while (pos < data.size)
		{
			switch (blockHeader->type)
			{
				case BMF_Block_Info: {
					// Ignored
				} break;

				case BMF_Block_Common: {
					common = (BMFontBlock_Common *)(data.memory + pos);
				} break;

				case BMF_Block_Pages: {
					pages = data.memory + pos;
				} break;

				case BMF_Block_Chars: {
					chars = (BMFont_Char *)(data.memory + pos);
					charCount = blockHeader->size / sizeof(BMFont_Char);
				} break;

				case BMF_Block_KerningPairs: {
					// Ignored for now
				} break;
			}

			pos += blockHeader->size;

			blockHeader = (BMFontBlockHeader *)(data.memory + pos);
			pos += sizeof(BMFontBlockHeader);
		}

		if (! (common && chars && charCount && pages) )
		{
			// Something didn't load correctly!
			logError("BMFont file '{0}' seems to be lacking crucial data and could not be loaded!", {asset->fullName});
		}
		else
		{
			BitmapFont *font = &asset->bitmapFont;
			font->lineHeight = common->lineHeight;
			font->baseY = common->base;
			font->nullGlyph = {};
			font->glyphCount = 0;

			smm pageMemorySize = common->pageCount * sizeof(font->pageTextures[0]);
			font->glyphCapacity = charCount * fontGlyphCapacityMultiplier;
			smm glyphEntryMemorySize = font->glyphCapacity * sizeof(BitmapFontGlyphEntry);
			asset->data = allocate(assets, pageMemorySize + glyphEntryMemorySize);

			font->pageTextures = (Asset **)(asset->data.memory);
			font->glyphEntries = (BitmapFontGlyphEntry *)(asset->data.memory + pageMemorySize);

			font->pageCount = common->pageCount;
			char *pageStart = (char *) pages;
			for (u32 pageIndex = 0;
				pageIndex < common->pageCount;
				pageIndex++)
			{
				String textureName = pushString(&assets->assetArena, pageStart);
				font->pageTextures[pageIndex] = addTexture(assets, textureName, false);
				pageStart += strlen(pageStart) + 1;
			}

			font->spriteGroup = addSpriteGroup(assets, asset->shortName, charCount);

			for (u32 charIndex = 0;
				charIndex < charCount;
				charIndex++)
			{
				BMFont_Char *src = chars + charIndex;

				BitmapFontGlyph *dest = addGlyph(font, src->id);

				dest->codepoint = src->id;
				dest->size = irectXYWH(src->x, src->y, src->w, src->h);
				dest->xOffset = src->xOffset;
				dest->yOffset = src->yOffset;
				dest->xAdvance = src->xAdvance;
				dest->page = src->page;
				dest->uv = rectXYWH( (f32)src->x, (f32)src->y, (f32)src->w, (f32)src->h);

				dest->sprite = font->spriteGroup->spriteGroup.sprites + charIndex;
				dest->sprite->texture = font->pageTextures[src->page];
				dest->sprite->uv = rectXYWH( (f32)src->x, (f32)src->y, (f32)src->w, (f32)src->h);
			}
		}
	}
}
