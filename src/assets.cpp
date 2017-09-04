#pragma once

int32 addTexture(AssetManager *assets, String filename, bool isAlphaPremultiplied)
{
	TextureList *list = assets->firstTextureList.prev;
	if (list->usedCount >= ArrayCount(list->textures))
	{
		list = PushStruct(&assets->assetArena, TextureList);
		DLinkedListInsertBefore(list, &assets->firstTextureList);
	}

	uint32 idWithinList = list->usedCount++;
	uint32 textureID = assets->textureCount++;
	ASSERT(idWithinList == (textureID % ArrayCount(list->textures)), "Texture index mismatch!");

	Texture *texture = list->textures + idWithinList;

	texture->filename = pushString(&assets->assetArena, filename);
	texture->isAlphaPremultiplied = isAlphaPremultiplied;

	return textureID;
}

uint32 addTextureRegion(AssetManager *assets, TextureAssetType type, int32 textureID, Rect2 uv)
{
	TextureRegionList *list = assets->firstTextureRegionList.prev;
	if (list->usedCount >= ArrayCount(list->regions))
	{
		list = PushStruct(&assets->assetArena, TextureRegionList);
		DLinkedListInsertBefore(list, &assets->firstTextureRegionList);
	}

	uint32 idWithinList = list->usedCount++;
	uint32 textureRegionID = assets->textureRegionCount++;
	ASSERT(idWithinList == (textureRegionID % ArrayCount(list->regions)), "Region index mismatch!");

	TextureRegion *region = list->regions + idWithinList;

	region->type = type;
	region->textureID = textureID;
	region->uv = uv;

	assets->firstIDForTextureAssetType[type] = min(textureRegionID, assets->firstIDForTextureAssetType[type]);
	assets->lastIDForTextureAssetType[type] = max(textureRegionID, assets->lastIDForTextureAssetType[type]);

	return textureRegionID;
}

void initAssetManager(AssetManager *assets)
{
	assets->assetsPath = pushString(&assets->assetArena, "assets");

	DLinkedListInit(&assets->firstTextureRegionList);
	DLinkedListInit(&assets->firstTextureList);

	assets->firstTextureList.usedCount = assets->textureCount = 0;
	assets->firstTextureRegionList.usedCount = assets->textureRegionCount = 0;

	addTextureRegion(assets, TextureAssetType_None, -1, {});

	Texture *nullTexture = getTexture(assets, addTexture(assets, {}, true));
	nullTexture->state = AssetState_Loaded;
	nullTexture->surface = 0;

	// Have to provide defaults for these or it just breaks.
	for (uint32 i = 0; i < TextureAssetTypeCount; ++i)
	{
		assets->firstIDForTextureAssetType[i] = uint32Max;
		assets->lastIDForTextureAssetType[i] = 0;
	}
}

AssetManager *createAssetManager()
{
	AssetManager *assets;
	bootstrapArena(AssetManager, assets, assetArena);

	initAssetManager(assets);

	return assets;
}

int32 findTexture(AssetManager *assets, String filename)
{
	int32 index = -1;
	for (int32 i = 0; i < (int32)assets->textureCount; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (equals(filename, tex->filename))
		{
			index = i;
			break;
		}
	}
	return index;
}

uint32 addTextureRegion(AssetManager *assets, TextureAssetType type, char *filename, Rect2 uv,
	                   bool isAlphaPremultiplied=false)
{
	String sFilename = pushString(&assets->assetArena, filename);
	int32 textureID = findTexture(assets, sFilename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, sFilename, isAlphaPremultiplied);
	}

	return addTextureRegion(assets, type, textureID, uv);
}

void addCursor(AssetManager *assets, CursorType cursorID, char *filename)
{
	Cursor *cursor = assets->cursors + cursorID;
	cursor->filename = pushString(&assets->assetArena, filename);
	cursor->sdlCursor = 0;
}

BitmapFont *addBMFont(AssetManager *assets, MemoryArena *tempArena, FontAssetType fontAssetType,
	                  TextureAssetType textureAssetType, char *filename);

void addShaderProgram(AssetManager *assets, ShaderProgramType shaderID, char *vertFilename,
	                  char *fragFilename)
{
	ShaderProgram *shader = assets->shaderPrograms + shaderID;
	shader->fragFilename = pushString(&assets->assetArena, fragFilename);
	shader->vertFilename = pushString(&assets->assetArena, vertFilename);
}

void loadAssets(AssetManager *assets)
{
	DEBUG_FUNCTION();
	for (uint32 i = 1; i < assets->textureCount; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (tex->state == AssetState_Unloaded)
		{
			tex->surface = IMG_Load(getAssetPath(assets, AssetType_Texture, tex->filename).chars);
			ASSERT(tex->surface, "Failed to load image '%*s'!\n%s", tex->filename.length, tex->filename.chars, IMG_GetError());

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
		TextureRegion *tr = getTextureRegion(assets, regionIndex);
		// NB: We look up the texture for every char, so fairly inefficient.
		// Maybe we could cache the current texture?
		Texture *t = getTexture(assets, tr->textureID);
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
	for (uint32 cursorID = 1; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = assets->cursors + cursorID;

		SDL_Surface *cursorSurface = IMG_Load(getAssetPath(assets, AssetType_Cursor, cursor->filename).chars);
		cursor->sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		SDL_FreeSurface(cursorSurface);
	}

	// Load shader programs
	for (uint32 shaderID = 0; shaderID < ShaderProgramCount; shaderID++)
	{
		ShaderProgram *shader = assets->shaderPrograms + shaderID;
		shader->vertShader = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, shader->vertFilename));
		shader->fragShader = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, shader->fragFilename));

		if (shader->vertShader.length && shader->fragShader.length)
		{
			shader->state = AssetState_Loaded;
		}
		else
		{
			ASSERT(false, "Failed to load shader program %d.", shaderID);
		}
	}

	// FIXME @Hack: hard-coded asset files, should be replaced with proper stuff later.
	loadUITheme(&assets->theme, readFileAsString(globalFrameTempArena, getAssetPath(assets, AssetType_Misc, stringFromChars("ui.theme"))));
	assets->creditsText = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Misc, stringFromChars("credits.txt")));
}

void addAssets(AssetManager *assets, MemoryArena *tempArena)
{
	addTextureRegion(assets, TextureAssetType_Map1, "London-Strand-Holbron-Bloomsbury.png",
	                 rectXYWH(0,0,2002,1519), false);
	addBMFont(assets, tempArena, FontAssetType_Buttons, TextureAssetType_Font_Buttons, "dejavu-14.fnt");
	addBMFont(assets, tempArena, FontAssetType_Main, TextureAssetType_Font_Main, "dejavu-20.fnt");

	addShaderProgram(assets, ShaderProgram_Textured, "textured.vert.gl", "textured.frag.gl");
	addShaderProgram(assets, ShaderProgram_Untextured, "untextured.vert.gl", "untextured.frag.gl");

	addCursor(assets, Cursor_Main, "cursor_main.png");
	addCursor(assets, Cursor_Build, "cursor_build.png");
	addCursor(assets, Cursor_Demolish, "cursor_demolish.png");
	addCursor(assets, Cursor_Plant, "cursor_plant.png");
	addCursor(assets, Cursor_Harvest, "cursor_harvest.png");
	addCursor(assets, Cursor_Hire, "cursor_hire.png");

#if BUILD_DEBUG
	addBMFont(assets, tempArena, FontAssetType_Debug, TextureAssetType_Font_Debug, "debug.fnt");
#endif
}

void reloadAssets(AssetManager *assets, MemoryArena *tempArena, Renderer *renderer, UIState *uiState)
{
	// Preparation
	consoleWriteLine("Reloading assets...");
	renderer->unloadAssets(renderer);
	SDL_Cursor *systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	SDL_SetCursor(systemWaitCursor);
	DEFER(SDL_FreeCursor(systemWaitCursor));

	// Actual reloading

	// Clear out textures
	for (uint32 i = 1; i < assets->textureCount; ++i)
	{
		Texture *tex = getTexture(assets, i);
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
	resetMemoryArena(&assets->assetArena);
	initAssetManager(assets);
	addAssets(assets, tempArena);
	loadAssets(assets);

	// After stuff
	setCursor(uiState, assets, uiState->currentCursor);
	renderer->loadAssets(renderer, assets);
	consoleWriteLine("Assets reloaded successfully!", CLS_Success);
}