#pragma once

AssetManager *createAssetManager()
{
	MemoryArena bootstrap;
	ASSERT(initMemoryArena(&bootstrap, MB(128)),"Failed to allocate asset memory!");
	AssetManager *assets = PushStruct(&bootstrap, AssetManager);
	assets->arena = bootstrap;

	assets->textureRegions[0].type = TextureAssetType_None;
	assets->textureRegions[0].textureID = -1;

	return assets;
}

int32 addTexture(AssetManager *assets, char *filename, bool isAlphaPremultiplied)
{
	ASSERT(assets->textureCount < ArrayCount(assets->textures), "No room for texture");

	int32 textureID = assets->textureCount++;

	Texture *texture = assets->textures + textureID;
	texture->filename = filename;
	texture->isAlphaPremultiplied = isAlphaPremultiplied;

	return textureID;
}

int32 findTexture(AssetManager *assets, char *filename)
{
	int32 index = -1;
	for (int32 i = 0; i < (int32)assets->textureCount; ++i)
	{
		Texture *tex = assets->textures + i;
		if (strcmp(filename, tex->filename) == 0)
		{
			index = i;
			break;
		}
	}
	return index;
}

TextureRegion *addTextureRegion(AssetManager *assets, TextureAssetType type, char *filename, RealRect uv, bool isAlphaPremultiplied=false)
{
	ASSERT(assets->textureRegionCount < ArrayCount(assets->textureRegions), "No room for texture region");
	int32 textureRegionID = assets->textureRegionCount++;
	TextureRegion *result = assets->textureRegions + textureRegionID;

	int32 textureID = findTexture(assets, filename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, filename, isAlphaPremultiplied);
	}

	result->type = type;
	result->textureID = textureID;
	result->uv = uv;

	assets->firstIDForTextureAssetType[type] = min(textureRegionID, assets->firstIDForTextureAssetType[type]);

	return result;
}

void loadTextures(AssetManager *assets)
{
	for (uint32 i = 0; i < assets->textureCount; ++i)
	{
		Texture *tex = assets->textures + i;
		if (tex->state == AssetState_Unloaded)
		{
			tex->surface = IMG_Load(tex->filename);
			ASSERT(tex->surface, "Failed to load image '%s'!\n%s", tex->filename, IMG_GetError());

			ASSERT(tex->surface->format->BytesPerPixel == 4, "We only handle 32-bit colour images!");
			if (!tex->isAlphaPremultiplied)
			{
				// Premultiply alpha
				uint32 Rmask = tex->surface->format->Rmask,
					   Gmask = tex->surface->format->Gmask,
					   Bmask = tex->surface->format->Bmask,
					   Amask = tex->surface->format->Amask;
				real32 rRmask = (real32)Rmask,
					   rGmask = (real32)Gmask,
					   rBmask = (real32)Bmask,
					   rAmask = (real32)Amask;

				uint32 pixelCount = tex->surface->w * tex->surface->h;
				for (uint32 pixelIndex=0;
					pixelIndex < pixelCount;
					pixelIndex++)
				{
					uint32 pixel = ((uint32*)tex->surface->pixels)[pixelIndex];
					real32 rr = (real32)(pixel & Rmask) / rRmask;
					real32 rg = (real32)(pixel & Gmask) / rGmask;
					real32 rb = (real32)(pixel & Bmask) / rBmask;
					real32 ra = (real32)(pixel & Amask) / rAmask;

					uint32 r = (uint32)(rr * ra * rRmask) & Rmask;
					uint32 g = (uint32)(rg * ra * rGmask) & Gmask;
					uint32 b = (uint32)(rb * ra * rBmask) & Bmask;
					uint32 a = (uint32)(ra * rAmask) & Amask;

					((uint32*)tex->surface->pixels)[pixelIndex] = (uint32)r | (uint32)g | (uint32)b | (uint32)a;
				}
				tex->isAlphaPremultiplied = true;
			}

			tex->state = AssetState_Loaded;
		}
	}
}

int32 getTextureRegion(AssetManager *assets, TextureAssetType item, int32 offset)
{
	int32 min = assets->firstIDForTextureAssetType[item],
		  max = assets->lastIDForTextureAssetType[item];

	int32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a textureRegionId outside of the range.");

	return id;
}