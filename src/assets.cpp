#pragma once

AssetManager *createAssetManager()
{
	MemoryArena bootstrap;
	ASSERT(initMemoryArena(&bootstrap, MB(128)),"Failed to allocate asset memory!");
	AssetManager *assets = PushStruct(&bootstrap, AssetManager);
	assets->arena = bootstrap;

	assets->textureRegions[0].type = TextureAssetType_None;
	assets->textureRegions[0].textureID = -1;
	assets->textureRegionCount = 1;

	assets->textures[0].state = AssetState_Loaded;
	assets->textures[0].filename = "";
	assets->textures[0].isAlphaPremultiplied = true;
	assets->textures[0].surface = null;
	assets->textureCount = 1;

	// Have to provide defaults for these or it just breaks.
	for (int i = 0; i < TextureAssetTypeCount; ++i)
	{
		assets->firstIDForTextureAssetType[i] = int32Max;
		assets->lastIDForTextureAssetType[i] = int32Min;
	}

	return assets;
}

void initTheme(AssetManager *assets)
{
	assets->theme.buttonTextColor        = color255(   0,   0,   0, 255 );
	assets->theme.buttonBackgroundColor  = color255( 255, 255, 255, 255 );
	assets->theme.buttonHoverColor 	     = color255( 192, 192, 255, 255 );
	assets->theme.buttonPressedColor     = color255( 128, 128, 255, 255 );

	assets->theme.labelColor             = color255( 255, 255, 255, 255 );
	assets->theme.overlayColor           = color255(   0,   0,   0, 128 );

	assets->theme.textboxBackgroundColor = color255( 255, 255, 255, 255 );
	assets->theme.textboxTextColor       = color255(   0,   0,   0, 255 );
	
	assets->theme.tooltipBackgroundColor = color255(   0,   0,   0, 128 );
	assets->theme.tooltipColorNormal     = color255( 255, 255, 255, 255 );
	assets->theme.tooltipColorBad        = color255( 255,   0,   0, 255 );

	assets->theme.font       = FontAssetType_Main;
	assets->theme.buttonFont = FontAssetType_Buttons;
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

int32 addTextureRegion(AssetManager *assets, TextureAssetType type, int32 textureID, RealRect uv)
{
	ASSERT(assets->textureRegionCount < ArrayCount(assets->textureRegions), "No room for texture region");
	int32 textureRegionID = assets->textureRegionCount++;
	TextureRegion *region = assets->textureRegions + textureRegionID;

	region->type = type;
	region->textureID = textureID;
	region->uv = uv;

	assets->firstIDForTextureAssetType[type] = min(textureRegionID, assets->firstIDForTextureAssetType[type]);
	assets->lastIDForTextureAssetType[type] = max(textureRegionID, assets->lastIDForTextureAssetType[type]);

	return textureRegionID;
}

int32 addTextureRegion(AssetManager *assets, TextureAssetType type, char *filename, RealRect uv, bool isAlphaPremultiplied=false)
{
	int32 textureID = findTexture(assets, filename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, filename, isAlphaPremultiplied);
	}

	return addTextureRegion(assets, type, textureID, uv);
}

void loadAssets(AssetManager *assets)
{
	for (uint32 i = 1; i < assets->textureCount; ++i)
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
			}

			tex->state = AssetState_Loaded;
		}
	}

	// Now we can convert UVs from pixel space to 0-1 space.
	for (uint32 regionIndex = 1; regionIndex < assets->textureRegionCount; regionIndex++)
	{
		TextureRegion *tr = assets->textureRegions + regionIndex;
		// NB: We look up the texture for every char, so fairly inefficient.
		// Maybe we could cache the current texture?
		Texture *t = assets->textures + tr->textureID;
		real32 textureWidth = (real32) t->surface->w;
		real32 textureHeight = (real32) t->surface->h;

		tr->uv = rectXYWH(
			tr->uv.x / textureWidth,
			tr->uv.y / textureHeight,
			tr->uv.w / textureWidth,
			tr->uv.h / textureHeight
		);
	}

	// Load up our cursors
	for (uint32 cursorID = 0; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = assets->cursors + cursorID;

		SDL_Surface *cursorSurface = IMG_Load(cursor->filename);
		cursor->sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		SDL_FreeSurface(cursorSurface);
	}

	initTheme(assets);
}

void reloadAssets(AssetManager *assets)
{
	// Clear out textures
	for (uint32 i = 1; i < assets->textureCount; ++i)
	{
		Texture *tex = assets->textures + i;
		if (tex->state == AssetState_Loaded)
		{
			SDL_FreeSurface(tex->surface);
			tex->surface = 0;
		}
		tex->state = AssetState_Unloaded;
	}

	// Clear fonts
	// Allocations are from assets arena so they get cleared below.
	for (int i = 0; i < FontAssetTypeCount; i++)
	{
		BitmapFont *font = assets->fonts + i;
		*font = {};
	}


	// Clear cursors
	for (uint32 cursorID = 0; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = assets->cursors + cursorID;
		SDL_FreeCursor(cursor->sdlCursor);
		cursor->sdlCursor = 0;
	}

	// General resetting of Assets system
	resetMemoryArena(&assets->arena);

	loadAssets(assets);
}

int32 getTextureRegion(AssetManager *assets, TextureAssetType item, int32 offset)
{
	int32 min = assets->firstIDForTextureAssetType[item],
		  max = assets->lastIDForTextureAssetType[item];

	int32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a textureRegionId outside of the range.");

	return id;
}

BitmapFont *getFont(AssetManager *assets, FontAssetType font)
{
	return assets->fonts + font;
}

void addCursor(AssetManager *assets, CursorType cursorID, char *filename)
{
	Cursor *cursor = assets->cursors + cursorID;
	cursor->filename = filename;
	cursor->sdlCursor = 0;
}

void setCursor(AssetManager *assets, CursorType cursorID)
{
	SDL_SetCursor(assets->cursors[cursorID].sdlCursor);
}