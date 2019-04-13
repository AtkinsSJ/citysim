#pragma once

void initAssetManager(AssetManager *assets)
{
	*assets = {};
	assets->assetsPath = pushString(&assets->assetArena, "assets");

	char *userDataPath = SDL_GetPrefPath("Baffled Badger Games", "CitySim");
	assets->userDataPath = pushString(&assets->assetArena, userDataPath);
	SDL_free(userDataPath);

	initChunkedArray(&assets->allAssets, &assets->assetArena, 2048);
	assets->assetMemoryAllocated = 0;
	assets->maxAssetMemoryAllocated = 0;

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

	initChunkedArray(&assets->cursorTypeToAssetIndex, &assets->assetArena, CursorCount, true);
	initChunkedArray(&assets->shaderProgramTypeToAssetIndex, &assets->assetArena, ShaderProgramCount, true);

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

s32 addAsset(AssetManager *assets, AssetType type, char *shortName)
{
	s32 index = assets->allAssets.count;

	Asset *asset = appendBlank(&assets->allAssets);
	asset->type = type;
	if (shortName != null)
	{
		asset->shortName = pushString(&assets->assetArena, shortName);
		asset->fullName = getAssetPath(assets, asset->type, asset->shortName);
	}
	asset->state = AssetState_Unloaded;
	asset->size = 0;
	asset->memory = null;

	return index;
}

void loadAsset(AssetManager *assets, Asset *asset)
{
	ASSERT(asset->state == AssetState_Unloaded, "Attempted to load an asset ({0}) that is already loaded!", {asset->fullName});

	// Some assets (meta-assets?) have no file associated with them, because they are composed of other assets.
	// eg, ShaderPrograms are made of several ShaderParts.
	if (asset->fullName.length > 0)
	{
		FileHandle file = openFile(asset->fullName, FileAccess_Read);

		smm fileSize = getFileSize(&file);
		asset->memory = allocateRaw(fileSize);

		asset->size = fileSize;

		smm bytesRead = readFileIntoMemory(&file, fileSize, asset->memory);
		closeFile(&file);

		assets->assetMemoryAllocated += bytesRead;
		assets->maxAssetMemoryAllocated = max(assets->assetMemoryAllocated, assets->maxAssetMemoryAllocated);

		if (bytesRead != fileSize)
		{
			logError("File {0} was only partially loaded. Size {1}, loaded {2}", {asset->fullName, formatInt(fileSize), formatInt(bytesRead)});
		}
		else
		{
			asset->state = AssetState_Loaded;
		}
	}

	// Type-specific loading
	switch (asset->type)
	{
		case AssetType_Cursor:
		{
			SDL_RWops *rw = SDL_RWFromConstMem(asset->memory, asset->size);
			SDL_Surface *cursorSurface = IMG_Load_RW(rw, 0);
			asset->cursor.sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
			SDL_FreeSurface(cursorSurface);
			SDL_RWclose(rw);
		} break;

		case AssetType_ShaderProgram:
		{
			Asset *frag = getAsset(assets, asset->shaderProgram.fragShaderAssetIndex);
			Asset *vert = getAsset(assets, asset->shaderProgram.vertShaderAssetIndex);
			if ((frag->state == AssetState_Loaded) && (vert->state == AssetState_Loaded))
			{
				asset->state = AssetState_Loaded;
			}
			else
			{
				logError("ShaderProgram {0} cannot be loaded because 1 or more of its component parts are not loaded. {1}: Loaded={2}, {3}: Loaded={4}", {
					asset->shortName,
					frag->shortName, formatBool(frag->state == AssetState_Loaded),
					vert->shortName, formatBool(vert->state == AssetState_Loaded)
				});
			}
		} break;

		case AssetType_UITheme:
		{
			loadUITheme(assets, asset);
		} break;

		case AssetType_BuildingDefs:
		{
			loadBuildingDefs(&buildingDefs, assets, asset);
		} break;

		case AssetType_TerrainDefs:
		{
			loadTerrainDefinitions(&terrainDefs, assets, asset);
		} break;
	}
}

void freeAsset(AssetManager *assets, Asset *asset)
{
	if (asset->state == AssetState_Unloaded) return;

	switch (asset->type)
	{
		case AssetType_Cursor:
		{
			if (asset->cursor.sdlCursor != null)
			{
				SDL_FreeCursor(asset->cursor.sdlCursor);
				asset->cursor.sdlCursor = null;
			}
		} break;
	}

	assets->assetMemoryAllocated -= asset->size;
	asset->size = 0;
	free(asset->memory);
	asset->state = AssetState_Unloaded;
}

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
	s32 assetIndex = addAsset(assets, AssetType_Cursor, filename);
	*get(&assets->cursorTypeToAssetIndex, cursorID) = assetIndex;
}

void addShaderHeader(AssetManager *assets, char *filename)
{
	assets->shaderHeaderAssetIndex = addAsset(assets, AssetType_ShaderPart, filename);
}

void addShaderProgram(AssetManager *assets, ShaderProgramType shaderID, char *vertFilename,
	                  char *fragFilename)
{
	// NB: For now, we have to load the part assets before the program asset, so they must come first
	s32 fragShaderAssetIndex = addAsset(assets, AssetType_ShaderPart, fragFilename);
	s32 vertShaderAssetIndex = addAsset(assets, AssetType_ShaderPart, vertFilename);

	s32 assetIndex = addAsset(assets, AssetType_ShaderProgram, null);
	*get(&assets->shaderProgramTypeToAssetIndex, shaderID) = assetIndex;

	Asset *asset = getAsset(assets, assetIndex);
	asset->shaderProgram.fragShaderAssetIndex = fragShaderAssetIndex;
	asset->shaderProgram.vertShaderAssetIndex = vertShaderAssetIndex;
}

void loadAssets(AssetManager *assets)
{
	DEBUG_FUNCTION();

	File defaultSettingsFile = readFile(globalFrameTempArena, getAssetPath(assets, AssetType_Misc, makeString("default-settings.cnf")));
	File userSettingsFile = readFile(globalFrameTempArena, getUserSettingsPath(assets));
	loadSettings(&globalAppState.settings, defaultSettingsFile, userSettingsFile);

	// TEMP: Test saving
	saveSettings(&globalAppState.settings, assets);

	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		loadAsset(assets, asset);
	}

	for (s32 i = 1; i < assets->textures.count; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (tex->state == AssetState_Unloaded)
		{
			tex->surface = IMG_Load(getAssetPath(assets, AssetType_Texture, tex->filename).chars);
			ASSERT(tex->surface != null, "Failed to load image '{0}'!\n{1}", {tex->filename, makeString(IMG_GetError())});

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
	assets->creditsAssetIndex = addAsset(assets, AssetType_Misc, "credits.txt");
	addAsset(assets, AssetType_UITheme,      "ui.theme");
	addAsset(assets, AssetType_BuildingDefs, "buildings.def");
	addAsset(assets, AssetType_TerrainDefs,  "terrain.def");

	// TODO: Settings?


	// addBMFont(assets, FontAssetType_Buttons, "dejavu-14.fnt");
	// addBMFont(assets, FontAssetType_Main, "dejavu-20.fnt");

	addShaderHeader(assets, "header.glsl");
	addShaderProgram(assets, ShaderProgram_Textured,   "textured.vert.glsl",   "textured.frag.glsl");
	addShaderProgram(assets, ShaderProgram_Untextured, "untextured.vert.glsl", "untextured.frag.glsl");
	addShaderProgram(assets, ShaderProgram_PixelArt,   "pixelart.vert.glsl",   "pixelart.frag.glsl");

	addCursor(assets, Cursor_Main,     "cursor_main.png");
	addCursor(assets, Cursor_Build,    "cursor_build.png");
	addCursor(assets, Cursor_Demolish, "cursor_demolish.png");
	addCursor(assets, Cursor_Plant,    "cursor_plant.png");
	addCursor(assets, Cursor_Harvest,  "cursor_harvest.png");
	addCursor(assets, Cursor_Hire,     "cursor_hire.png");

#if BUILD_DEBUG
	addBMFont(assets, makeString("debug"), makeString("debug.fnt"));
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

	// Clear managed assets
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		freeAsset(assets, asset);
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