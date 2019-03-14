#pragma once

u32 addTexture(AssetManager *assets, String filename, bool isAlphaPremultiplied)
{
	u32 textureID = assets->textures.count;

	Texture *texture = appendBlank(&assets->textures);

	texture->state = AssetState_Unloaded;
	texture->filename = pushString(&assets->assetArena, filename);
	texture->isAlphaPremultiplied = isAlphaPremultiplied;

	return textureID;
}

u32 addTextureRegion(AssetManager *assets, u32 textureRegionAssetType, s32 textureID, Rect2 uv)
{
	u32 textureRegionID = assets->textureRegions.count;

	TextureRegion *region = appendBlank(&assets->textureRegions);

	region->textureRegionAssetType = textureRegionAssetType;
	region->textureID = textureID;
	region->uv = uv;

	IndexRange *range = get(&assets->rangesByTextureAssetType, textureRegionAssetType);
	range->firstIndex = MIN(textureRegionID, range->firstIndex);
	range->lastIndex  = MAX(textureRegionID, range->lastIndex);

	return textureRegionID;
}

void initAssetManager(AssetManager *assets)
{
	*assets = {};
	assets->assetsPath = pushString(&assets->assetArena, "assets");

	initChunkedArray(&assets->rangesByTextureAssetType, &assets->assetArena, 32);
	appendBlank(&assets->rangesByTextureAssetType);

	initChunkedArray(&assets->textures, &assets->assetArena, 32);
	Texture *nullTexture = appendBlank(&assets->textures);
	nullTexture->state = AssetState_Loaded;
	nullTexture->surface = null;
	nullTexture->isAlphaPremultiplied = true;

	initChunkedArray(&assets->textureRegions, &assets->assetArena, 512);
	TextureRegion *nullRegion = appendBlank(&assets->textureRegions);
	nullRegion->textureID = -1;

	initChunkedArray(&assets->cursors, &assets->assetArena, 16, true);
	initChunkedArray(&assets->shaderPrograms, &assets->assetArena, 16, true);

	// Stuff that used to be in the UI theme is now here... I think UITheme isn't a useful concept?
	initChunkedArray(&assets->fonts,         &assets->assetArena, 16);
	initChunkedArray(&assets->buttonStyles,  &assets->assetArena, 16);
	initChunkedArray(&assets->labelStyles,   &assets->assetArena, 16);
	initChunkedArray(&assets->tooltipStyles, &assets->assetArena, 16);
	initChunkedArray(&assets->messageStyles, &assets->assetArena, 16);
	initChunkedArray(&assets->textBoxStyles, &assets->assetArena, 16);
	initChunkedArray(&assets->windowStyles,  &assets->assetArena, 16);
}

AssetManager *createAssetManager()
{
	AssetManager *assets;
	bootstrapArena(AssetManager, assets, assetArena);

	initAssetManager(assets);

	return assets;
}

s32 findTexture(AssetManager *assets, String filename)
{
	s32 index = -1;
	for (s32 i = 0; i < (s32)assets->textures.count; ++i)
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

s32 addTextureRegion(AssetManager *assets, u32 textureRegionAssetType, char *filename, Rect2 uv,
	                   bool isAlphaPremultiplied=false)
{
	String sFilename = pushString(&assets->assetArena, filename);
	s32 textureID = findTexture(assets, sFilename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, sFilename, isAlphaPremultiplied);
	}

	return addTextureRegion(assets, textureRegionAssetType, textureID, uv);
}

void addCursor(AssetManager *assets, CursorType cursorID, char *filename)
{
	Cursor *cursor = get(&assets->cursors, cursorID);
	cursor->filename = pushString(&assets->assetArena, filename);
	cursor->sdlCursor = 0;
}

void addShaderHeader(AssetManager *assets, char *filename)
{
	ShaderHeader *shaderHeader = &assets->shaderHeader;
	shaderHeader->state = AssetState_Unloaded;
	shaderHeader->filename = pushString(&assets->assetArena, filename);
}

void addShaderProgram(AssetManager *assets, ShaderProgramType shaderID, char *vertFilename,
	                  char *fragFilename)
{
	ShaderProgram *shader = get(&assets->shaderPrograms, shaderID);
	shader->state = AssetState_Unloaded;
	shader->fragFilename = pushString(&assets->assetArena, fragFilename);
	shader->vertFilename = pushString(&assets->assetArena, vertFilename);
}

void loadAssets(AssetManager *assets)
{
	DEBUG_FUNCTION();

	// FIXME @Hack: hard-coded asset files, should be replaced with proper stuff later.
	loadUITheme(assets, readFile(globalFrameTempArena, getAssetPath(assets, AssetType_Misc, stringFromChars("ui.theme"))));
	assets->creditsText = readFile(&assets->assetArena, getAssetPath(assets, AssetType_Misc, stringFromChars("credits.txt")));
	loadBuildingDefs(&buildingDefs, assets, readFile(globalFrameTempArena, getAssetPath(assets, AssetType_Misc, stringFromChars("buildings.def"))));
	loadTerrainDefinitions(&terrainDefs, assets, readFile(globalFrameTempArena, getAssetPath(assets, AssetType_Misc, stringFromChars("terrain.def"))));

	for (s32 i = 1; i < assets->textures.count; ++i)
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
				u32 Rmask = tex->surface->format->Rmask,
					   Gmask = tex->surface->format->Gmask,
					   Bmask = tex->surface->format->Bmask,
					   Amask = tex->surface->format->Amask;
				f32 rRmask = (f32)Rmask,
					   rGmask = (f32)Gmask,
					   rBmask = (f32)Bmask,
					   rAmask = (f32)Amask;

				u32 pixelCount = tex->surface->w * tex->surface->h;
				for (u32 pixelIndex=0;
					pixelIndex < pixelCount;
					pixelIndex++)
				{
					u32 pixel = ((u32*)tex->surface->pixels)[pixelIndex];
					f32 rr = (f32)(pixel & Rmask) / rRmask;
					f32 rg = (f32)(pixel & Gmask) / rGmask;
					f32 rb = (f32)(pixel & Bmask) / rBmask;
					f32 ra = (f32)(pixel & Amask) / rAmask;

					u32 r = (u32)(rr * ra * rRmask) & Rmask;
					u32 g = (u32)(rg * ra * rGmask) & Gmask;
					u32 b = (u32)(rb * ra * rBmask) & Bmask;
					u32 a = (u32)(ra * rAmask) & Amask;

					((u32*)tex->surface->pixels)[pixelIndex] = (u32)r | (u32)g | (u32)b | (u32)a;
				}
			}

			tex->state = AssetState_Loaded;
		}
	}

	// Now we can convert UVs from pixel space to 0-1 space.
	for (s32 regionIndex = 1; regionIndex < assets->textureRegions.count; regionIndex++)
	{
		TextureRegion *tr = getTextureRegion(assets, regionIndex);
		// NB: We look up the texture for every char, so fairly inefficient.
		// Maybe we could cache the current texture?
		Texture *t = getTexture(assets, tr->textureID);
		f32 textureWidth = (f32) t->surface->w;
		f32 textureHeight = (f32) t->surface->h;

		tr->uv = rectXYWH(
			tr->uv.x / textureWidth,
			tr->uv.y / textureHeight,
			tr->uv.w / textureWidth,
			tr->uv.h / textureHeight
		);
	}

	// Load up our cursors
	for (u32 cursorID = 1; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = get(&assets->cursors, cursorID);

		SDL_Surface *cursorSurface = IMG_Load(getAssetPath(assets, AssetType_Cursor, cursor->filename).chars);
		cursor->sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		SDL_FreeSurface(cursorSurface);
	}

	// Load shader programs
	ShaderHeader *shaderHeader = &assets->shaderHeader;
	shaderHeader->contents = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, shaderHeader->filename));
	if (shaderHeader->contents.length)
	{
		shaderHeader->state = AssetState_Loaded;
	}
	else
	{
		logError("Failed to load shader header file {0}", {shaderHeader->filename});
	}

	for (u32 shaderID = 0; shaderID < ShaderProgramCount; shaderID++)
	{
		ShaderProgram *shader = get(&assets->shaderPrograms, shaderID);
		shader->vertShader = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, shader->vertFilename));
		shader->fragShader = readFileAsString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, shader->fragFilename));

		if (shader->vertShader.length && shader->fragShader.length)
		{
			shader->state = AssetState_Loaded;
		}
		else
		{
			logError("Failed to load shader program {0}", {formatInt(shaderID)});
		}
	}

	logInfo("Loaded {0} texture regions and {1} textures.", {formatInt(assets->textureRegions.count), formatInt(assets->textures.count)});

	assets->assetReloadHasJustHappened = true;
}

u32 addNewTextureAssetType(AssetManager *assets)
{
	u32 newTypeID = assets->rangesByTextureAssetType.count;

	IndexRange range;
	range.firstIndex = u32Max;
	range.lastIndex  = 0;
	append(&assets->rangesByTextureAssetType, range);

	return newTypeID;
}

void addTiledTextureRegions(AssetManager *assets, u32 textureAssetType, String filename, u32 tileWidth, u32 tileHeight, u32 tilesAcross, u32 tilesDown, bool isAlphaPremultiplied=false)
{
	String sFilename = pushString(&assets->assetArena, filename);
	s32 textureID = findTexture(assets, sFilename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, sFilename, isAlphaPremultiplied);
	}

	Rect2 uv = rectXYWH(0, 0, (f32)tileWidth, (f32)tileHeight);

	u32 index = 0;
	for (u32 y = 0; y < tilesDown; y++)
	{
		uv.y = (f32)(y * tileHeight);

		for (u32 x = 0; x < tilesAcross; x++)
		{
			uv.x = (f32)(x * tileWidth);

			addTextureRegion(assets, textureAssetType, textureID, uv);
			index++;
		}
	}
}

void addAssets(AssetManager *assets)
{
	// addBMFont(assets, FontAssetType_Buttons, "dejavu-14.fnt");
	// addBMFont(assets, FontAssetType_Main, "dejavu-20.fnt");

	addShaderHeader(assets, "header.glsl");
	addShaderProgram(assets, ShaderProgram_Textured,   "textured.vert.glsl",   "textured.frag.glsl");
	addShaderProgram(assets, ShaderProgram_Untextured, "untextured.vert.glsl", "untextured.frag.glsl");
	addShaderProgram(assets, ShaderProgram_PixelArt,   "textured.vert.glsl",   "pixelart.frag.glsl");

	// NB: These have to be in the same order as the CursorType right now!
	addCursor(assets, Cursor_Main,     "cursor_main.png");
	addCursor(assets, Cursor_Build,    "cursor_build.png");
	addCursor(assets, Cursor_Demolish, "cursor_demolish.png");
	addCursor(assets, Cursor_Plant,    "cursor_plant.png");
	addCursor(assets, Cursor_Harvest,  "cursor_harvest.png");
	addCursor(assets, Cursor_Hire,     "cursor_hire.png");

#if BUILD_DEBUG
	addBMFont(assets, stringFromChars("debug"), stringFromChars("debug.fnt"));
#endif
}

void reloadAssets(AssetManager *assets, Renderer *renderer, UIState *uiState)
{
	// Preparation
	consoleWriteLine("Reloading assets...");
	renderer->unloadAssets(renderer);
	SDL_Cursor *systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	SDL_SetCursor(systemWaitCursor);
	defer { SDL_FreeCursor(systemWaitCursor); };

	globalDebugState->font = null;
	globalConsole->font = null;

	// Actual reloading

	// Clear out textures
	for (s32 i = 1; i < assets->textures.count; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (tex->state == AssetState_Loaded)
		{
			SDL_FreeSurface(tex->surface);
			tex->surface = 0;
		}
		tex->state = AssetState_Unloaded;
	}

	// Clear cursors
	for (u32 cursorID = 0; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = get(&assets->cursors, cursorID);
		SDL_FreeCursor(cursor->sdlCursor);
		cursor->sdlCursor = 0;
	}

	// General resetting of Assets system
	// The "throw everything away and start over" method of reloading. It's dumb but effective!
	resetMemoryArena(&assets->assetArena);
	initAssetManager(assets);
	addAssets(assets);
	loadAssets(assets);

	// After stuff
	setCursor(uiState, uiState->currentCursor);
	renderer->loadAssets(renderer, assets);
	consoleWriteLine("Assets reloaded successfully!", CLS_Success);
}