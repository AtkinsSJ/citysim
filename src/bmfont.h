#pragma once

// See: http://www.angelcode.com/products/bmfont/doc/file_format.html

#pragma pack(push, 1)
struct BMFontHeader
{
	uint8 tag[3];
	uint8 version;
};
const uint8 BMFontTag[3] = {'B', 'M', 'F'};
const uint8 BMFontSupportedVersion = 3;

struct BMFontBlockHeader
{
	uint8 type;
	uint32 size;
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
	uint16 lineHeight;
	uint16 base; // Distance from top of line to the character baseline
	uint16 scaleW, scaleH; // Texture dimensions
	uint16 pageCount; // How many texture pages?

	uint8 bitfield; // 1 if 8-bit character data is packed into all channels
	uint8 alphaChannel;
	uint8 redChannel;
	uint8 greenChannel;
	uint8 blueChannel;
};

struct BMFont_Char
{
	uint32 id;
	uint16 x, y;
	uint16 w, h;
	int16 xOffset, yOffset; // Offset when rendering to the screen
	int16 xAdvance; // How far to move after rendering this character
	uint8 page; // Texture page
	uint8 channel; // Bitfield, 1 = blue, 2 = green, 4 = red, 8 = alpha
};

#pragma pack(pop)

BitmapFont *readBMFont(MemoryArena *renderArena, MemoryArena *TempArena, const char *filename, TexturesToLoad *texturesToLoad)
{
	BitmapFont *Font = 0;

	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	if (!file)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to open %s: %s", filename, SDL_GetError());
	}
	else
	{
		// Read whole file in first
		int64 fileLength = file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		ASSERT(fileLength <= INT32_MAX, "File is too big to fit into an int32!");
		size_t fileLength_t = (size_t)fileLength;

		uint8 *fileData = (uint8 *)allocate(TempArena, fileLength_t);
		if (!file->read(file, fileData, fileLength_t, 1))
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to read %s: %s", filename, SDL_GetError());
		}
		else
		{
			int64 pos = 0;
			BMFontHeader *header = (BMFontHeader *)(fileData + pos);
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
				uint32 charCount = 0;
				void *pages = 0;
					
				blockHeader = (BMFontBlockHeader *)(fileData + pos);
				pos += sizeof(BMFontBlockHeader);

				while (pos < fileLength)
				{
					switch (blockHeader->type)
					{
						case BMF_Block_Info: {
							// Ignored
						} break;

						case BMF_Block_Common: {
							common = (BMFontBlock_Common *)(fileData + pos);
						} break;

						case BMF_Block_Pages: {
							pages = fileData + pos;
						} break;

						case BMF_Block_Chars: {
							chars = (BMFont_Char *)(fileData + pos);
							charCount = blockHeader->size / sizeof(BMFont_Char);
						} break;

						case BMF_Block_KerningPairs: {
							// Ignored for now
						} break;
					}

					pos += blockHeader->size;

					blockHeader = (BMFontBlockHeader *)(fileData + pos);
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
					// Buffer-up the texture pages
					GLint *pageToTexture = PushArray(renderArena, GLint, common->pageCount);
					char *pageStart = (char *) pages;

					for (uint32 pageIndex = 0;
						pageIndex < common->pageCount;
						pageIndex++)
					{
						pageToTexture[pageIndex] = pushTextureToLoad(texturesToLoad, pageStart, true);
						pageStart += strlen(pageStart) + 1;
					}

					Font = PushStruct(renderArena, BitmapFont);
					Font->lineHeight = common->lineHeight;
					Font->baseY = common->base;

					Font->charCount = charCount;
					Font->chars = PushArray(renderArena, BitmapFontChar, charCount);

					const real32 textureWidth  = (real32) TEXTURE_WIDTH,
								 textureHeight = (real32) TEXTURE_HEIGHT;

					for (uint32 charIndex = 0;
						charIndex < charCount;
						charIndex++)
					{
						BMFont_Char *src = chars + charIndex;
						BitmapFontChar *dest = Font->chars + charIndex;

						dest->id = src->id;
						dest->size = irectXYWH(src->x, src->y, src->w, src->h);
						dest->xOffset = src->xOffset;
						dest->yOffset = src->yOffset;
						dest->xAdvance = src->xAdvance;

						dest->textureID = pageToTexture[src->page];
						// NB: If we start packing multiple images into a single texture at runtime,
						// this will be WRONG!!!!
						dest->uv = rectXYWH(
							(real32)src->x / textureWidth,
							(real32)src->y / textureHeight,
							(real32)src->w / textureWidth,
							(real32)src->h / textureHeight
						);
					}
				}
			}
		}

		file->close(file);
	}

	return Font;
}