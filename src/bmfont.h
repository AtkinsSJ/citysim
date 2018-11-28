#pragma once

// See: http://www.angelcode.com/products/bmfont/doc/file_format.html

#pragma pack(push, 1)
struct BMFontHeader
{
	u8 tag[3];
	u8 version;
};
const u8 BMFontTag[3] = {'B', 'M', 'F'};
const u8 BMFontSupportedVersion = 3;

struct BMFontBlockHeader
{
	u8 type;
	u32 size;
};
enum BMFontBlockTypes
{
	BMF_Block_Info = 1,
	BMF_Block_Common = 2,
	BMF_Block_Pages = 3,
	BMF_Block_Chars = 4,
	BMF_Block_KerningPairs = 5,
};

enum BMFont_ColorChannelType
{
	BMFont_CCT_Glyph = 0,
	BMFont_CCT_Outline = 1,
	BMFont_CCT_Combined = 2, // Glyph and outline
	BMFont_CCT_Zero = 3,
	BMFont_CCT_One = 4,
};
struct BMFontBlock_Common
{
	u16 lineHeight;
	u16 base; // Distance from top of line to the character baseline
	u16 scaleW, scaleH; // GL_Texture dimensions
	u16 pageCount; // How many texture pages?

	u8 bitfield; // 1 if 8-bit character data is packed into all channels
	u8 alphaChannel;
	u8 redChannel;
	u8 greenChannel;
	u8 blueChannel;
};

struct BMFont_Char
{
	u32 id;
	u16 x, y;
	u16 w, h;
	s16 xOffset, yOffset; // Offset when rendering to the screen
	s16 xAdvance; // How far to move after rendering this character
	u8 page; // GL_Texture page
	u8 channel; // Bitfield, 1 = blue, 2 = green, 4 = red, 8 = alpha
};

#pragma pack(pop)

BitmapFont *addBMFont(AssetManager *assets, MemoryArena *tempArena, FontAssetType fontAssetType,
	                  TextureAssetType textureAssetType, char *filename)
{
	BitmapFont *font = 0;
	TemporaryMemory tempMemory = beginTemporaryMemory(tempArena);

	File file = readFile(tempMemory.arena, filename);
	if (!file.data)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to open font file %s: %s", filename, SDL_GetError());
	}
	else
	{
		s64 pos = 0;
		BMFontHeader *header = (BMFontHeader *)(file.data + pos);
		pos += sizeof(BMFontHeader);

		// Check it's a valid BMF
		if (header->tag[0] != BMFontTag[0]
			|| header->tag[1] != BMFontTag[1]
			|| header->tag[2] != BMFontTag[2])
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Not a valid BMFont file: %s", filename);
		}
		else if (header->version != 3)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "BMFont file version is unsupported: %s, wanted %d and got %d",
							filename, BMFontSupportedVersion, header->version);
		}
		else
		{
			BMFontBlockHeader *blockHeader = 0;
			BMFontBlock_Common *common = 0;
			BMFont_Char *chars = 0;
			u32 charCount = 0;
			void *pages = 0;
				
			blockHeader = (BMFontBlockHeader *)(file.data + pos);
			pos += sizeof(BMFontBlockHeader);

			while (pos < file.length)
			{
				switch (blockHeader->type)
				{
					case BMF_Block_Info: {
						// Ignored
					} break;

					case BMF_Block_Common: {
						common = (BMFontBlock_Common *)(file.data + pos);
					} break;

					case BMF_Block_Pages: {
						pages = file.data + pos;
					} break;

					case BMF_Block_Chars: {
						chars = (BMFont_Char *)(file.data + pos);
						charCount = blockHeader->size / sizeof(BMFont_Char);
					} break;

					case BMF_Block_KerningPairs: {
						// Ignored for now
					} break;
				}

				pos += blockHeader->size;

				blockHeader = (BMFontBlockHeader *)(file.data + pos);
				pos += sizeof(BMFontBlockHeader);
			}

			if (! (common && chars && charCount && pages) )
			{
				// Something didn't load correctly!
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
					"BMFont file '%s' seems to be lacking crucial data and could not be loaded!", filename);
			}
			else
			{
				s32 *pageToTextureID = PushArray(&tempMemory, s32, common->pageCount);
				char *pageStart = (char *) pages;

				for (u32 pageIndex = 0;
					pageIndex < common->pageCount;
					pageIndex++)
				{
					s32 textureID = addTexture(assets, pageStart, false);
					pageToTextureID[pageIndex] = textureID;
					pageStart += strlen(pageStart) + 1;
				}

				font = assets->fonts + fontAssetType;
				font->textureAssetType = textureAssetType;
				font->lineHeight = common->lineHeight;
				font->baseY = common->base;

				font->nullChar = {};

				font->charCount = charCount;
				font->chars = PushArray(&assets->assetArena, BitmapFontChar, charCount);

				for (u32 charIndex = 0;
					charIndex < charCount;
					charIndex++)
				{
					BMFont_Char *src = chars + charIndex;
					BitmapFontChar *dest = font->chars + charIndex;

					dest->codepoint = src->id;
					dest->size = irectXYWH(src->x, src->y, src->w, src->h);
					dest->xOffset = src->xOffset;
					dest->yOffset = src->yOffset;
					dest->xAdvance = src->xAdvance;

					dest->textureRegionID = addTextureRegion(assets, textureAssetType, pageToTextureID[src->page],
						rectXYWH( (f32)src->x, (f32)src->y, (f32)src->w, (f32)src->h));
				}
			}
		}
	}

	endTemporaryMemory(&tempMemory);

	return font;
}