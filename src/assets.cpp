#pragma once

AssetManager *createAssetManager()
{
	MemoryArena bootstrap;
	ASSERT(initMemoryArena(&bootstrap, MB(128)),"Failed to allocate asset memory!");
	AssetManager *assets = PushStruct(&bootstrap, AssetManager);
	assets->arena = bootstrap;

	return assets;
}

Texture *addTexture(AssetManager *assets, char *filename, bool isAlphaPremultiplied)
{
	ASSERT(assets->textureCount < ArrayCount(assets->textures), "No room for texture");

	Texture *result = assets->textures + assets->textureCount++;
	result->filename = filename;
	result->isAlphaPremultiplied = isAlphaPremultiplied;

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
			ASSERT(tex->surface, "Failed to load image '%s'!\n%s", filename, IMG_GetError());

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