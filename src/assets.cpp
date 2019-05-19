#pragma once

void initAssetManager(AssetManager *assets)
{
	*assets = {};
	assets->assetsPath = pushString(&assets->assetArena, "assets");

	char *userDataPath = SDL_GetPrefPath("Baffled Badger Games", "CitySim");
	assets->userDataPath = pushString(&assets->assetArena, userDataPath);
	SDL_free(userDataPath);

	// NB: We only need to define these for assets in the root assets/ directory
	// Well, for now at least.
	// - Sam, 19/05/2019
	initHashTable(&assets->fileExtensionToType);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "buildings"), AssetType_BuildingDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "terrain"), AssetType_TerrainDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "keymap"), AssetType_DevKeymap);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "theme"), AssetType_UITheme);

	initHashTable(&assets->directoryNameToType);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "cursors"), AssetType_Cursor);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "fonts"), AssetType_BitmapFont);
	// put(&assets->directoryNameToType, pushString(&assets->assetArena, "shaders"), AssetType_Shader);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "textures"), AssetType_Texture);

	initChunkedArray(&assets->allAssets, &assets->assetArena, 2048);
	assets->assetMemoryAllocated = 0;
	assets->maxAssetMemoryAllocated = 0;

	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		initHashTable(&assets->assetsByName[assetType]);
	}

	initUITheme(&assets->theme);
}

AssetManager *createAssetManager()
{
	AssetManager *assets;
	bootstrapArena(AssetManager, assets, assetArena);

	initAssetManager(assets);

	return assets;
}

Blob allocate(AssetManager *assets, smm size)
{
	Blob result = {};
	result.size = size;
	result.memory = allocateRaw(size);

	assets->assetMemoryAllocated += size;
	assets->maxAssetMemoryAllocated = max(assets->assetMemoryAllocated, assets->maxAssetMemoryAllocated);

	return result;
}

Asset *addAsset(AssetManager *assets, AssetType type, String shortName, bool isAFile=true)
{
	Asset *asset = appendBlank(&assets->allAssets);
	asset->type = type;
	if (shortName.length != 0)
	{
		asset->shortName = shortName;
		asset->fullName = getAssetPath(assets, asset->type, shortName);
	}
	asset->state = AssetState_Unloaded;
	asset->data.size = 0;
	asset->data.memory = null;
	asset->isAFile = isAFile;

	put(&assets->assetsByName[type], shortName, asset);

	return asset;
}

inline Asset *addAsset(AssetManager *assets, AssetType type, char *shortName, bool isAFile=true)
{
	String name = nullString;
	if (shortName != null) name = pushString(&assets->assetArena, shortName);
	return addAsset(assets, type, name, isAFile);
}

void copyFileIntoAsset(AssetManager *assets, Blob *fileData, Asset *asset)
{
	asset->data = allocate(assets, fileData->size);
	memcpy(asset->data.memory, fileData->memory, fileData->size);

	// NB: We set the fileData to point at the new copy, so that code after calling copyFileIntoAsset()
	// can still use fileData without having to mess with it. Already had one bug caused by not doing this!
	fileData->memory = asset->data.memory;
}

SDL_Surface *createSurfaceFromFileData(Blob fileData, String name)
{
	SDL_Surface *result = null;

	ASSERT(fileData.size > 0, "Attempted to create a surface from an unloaded asset! ({0})", {name});
	ASSERT(fileData.size < s32Max, "File '{0}' is too big for SDL's RWOps!", {name});

	SDL_RWops *rw = SDL_RWFromConstMem(fileData.memory, truncate32(fileData.size));
	if (rw)
	{
		result = IMG_Load_RW(rw, 0);

		ASSERT(result != null, "Failed to create SDL_Surface from asset '{0}'!\n{1}", {
			name, makeString(IMG_GetError())
		});

		SDL_RWclose(rw);
	}
	else
	{
		logError("Failed to create SDL_RWops from asset '{0}'!\n{1}", {
			name, makeString(SDL_GetError())
		});
	}

	return result;
}

void ensureAssetIsLoaded(AssetManager *assets, Asset *asset)
{
	if (asset->state == AssetState_Loaded) return;

	loadAsset(assets, asset);

	ASSERT(asset->state == AssetState_Loaded, "Failed to load asset '{0}'", {asset->shortName});
}

void loadAsset(AssetManager *assets, Asset *asset)
{
	DEBUG_FUNCTION();
	if (asset->state != AssetState_Unloaded) return;

	Blob fileData = {};
	// Some assets (meta-assets?) have no file associated with them, because they are composed of other assets.
	// eg, ShaderPrograms are made of several ShaderParts.
	if (asset->isAFile)
	{
		fileData = readTempFile(asset->fullName);
	}

	// Type-specific loading
	switch (asset->type)
	{
		case AssetType_BitmapFont:
		{
			loadBMFont(assets, fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_BuildingDefs:
		{
			loadBuildingDefs(&buildingDefs, assets, fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Cursor:
		{
			SDL_Surface *cursorSurface = createSurfaceFromFileData(fileData, asset->shortName);
			asset->cursor.sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
			SDL_FreeSurface(cursorSurface);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_DevKeymap:
		{
			if (globalConsole != null)
			{
				// NB: We keep the keymap file in the asset memory, so that the CommandShortcut.command can
				// directly refer to the string data from the file, instead of having to allocate a copy
				// and not be able to free it ever. This is more memory efficient.
				copyFileIntoAsset(assets, &fileData, asset);
				loadConsoleKeyboardShortcuts(globalConsole, fileData, asset->shortName);
			}
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Shader:
		{
			Blob vertexFile   = readTempFile(asset->shader.vertexShaderFilename);
			Blob fragmentFile = readTempFile(asset->shader.fragmentShaderFilename);
			
			smm totalSize = fragmentFile.size + vertexFile.size;
			asset->data = allocate(assets, totalSize);

			asset->shader.vertexShader   = makeString((char*)asset->data.memory, truncate32(vertexFile.size));
			asset->shader.fragmentShader = makeString((char*)asset->data.memory + vertexFile.size, truncate32(fragmentFile.size));

			memcpy(asset->data.memory,                   vertexFile.memory,   vertexFile.size);
			memcpy(asset->data.memory + vertexFile.size, fragmentFile.memory, fragmentFile.size);


			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Sprite:
		{
			// Convert UVs from pixel space to 0-1 space
			for (s32 i = 0; i < asset->spriteGroup.count; i++)
			{
				Sprite *sprite = asset->spriteGroup.sprites + i;
				Asset *t = sprite->texture;
				ensureAssetIsLoaded(assets, t);
				f32 textureWidth  = (f32) t->texture.surface->w;
				f32 textureHeight = (f32) t->texture.surface->h;

				sprite->uv = rectXYWH(
					sprite->uv.x / textureWidth,
					sprite->uv.y / textureHeight,
					sprite->uv.w / textureWidth,
					sprite->uv.h / textureHeight
				);
			}

			asset->state = AssetState_Loaded;
		} break;

		case AssetType_TerrainDefs:
		{
			loadTerrainDefinitions(&terrainDefs, assets, fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texture:
		{
			SDL_Surface *surface = createSurfaceFromFileData(fileData, asset->fullName);
			ASSERT(surface->format->BytesPerPixel == 4, "We only handle 32-bit colour images!");

			if (!asset->texture.isFileAlphaPremultiplied)
			{
				// Premultiply alpha
				u32 Rmask = surface->format->Rmask,
					Gmask = surface->format->Gmask,
					Bmask = surface->format->Bmask,
					Amask = surface->format->Amask;
				f32 rRmask = (f32)Rmask,
					rGmask = (f32)Gmask,
					rBmask = (f32)Bmask,
					rAmask = (f32)Amask;

				u32 pixelCount = surface->w * surface->h;
				for (u32 pixelIndex=0;
					pixelIndex < pixelCount;
					pixelIndex++)
				{
					u32 pixel = ((u32*)surface->pixels)[pixelIndex];
					f32 rr = (f32)(pixel & Rmask) / rRmask;
					f32 rg = (f32)(pixel & Gmask) / rGmask;
					f32 rb = (f32)(pixel & Bmask) / rBmask;
					f32 ra = (f32)(pixel & Amask) / rAmask;

					u32 r = (u32)(rr * ra * rRmask) & Rmask;
					u32 g = (u32)(rg * ra * rGmask) & Gmask;
					u32 b = (u32)(rb * ra * rBmask) & Bmask;
					u32 a = (u32)(ra * rAmask) & Amask;

					((u32*)surface->pixels)[pixelIndex] = (u32)r | (u32)g | (u32)b | (u32)a;
				}
			}

			asset->texture.surface = surface;	
			asset->state = AssetState_Loaded;	
		} break;

		case AssetType_UITheme:
		{
			loadUITheme(assets, fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		default:
		{
			copyFileIntoAsset(assets, &fileData, asset);
			asset->state = AssetState_Loaded;
		} break;
	}
}

void unloadAsset(AssetManager *assets, Asset *asset)
{
	DEBUG_FUNCTION();
	
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

		case AssetType_Texture:
		{
			if (asset->texture.surface != null)
			{
				SDL_FreeSurface(asset->texture.surface);
				asset->texture.surface = null;
			}
		} break;
	}

	if (asset->data.memory != null)
	{
		assets->assetMemoryAllocated -= asset->data.size;
		asset->data.size = 0;
		free(asset->data.memory);
	}

	asset->state = AssetState_Unloaded;
}

Asset *addTexture(AssetManager *assets, String filename, bool isAlphaPremultiplied)
{
	Asset *asset = addAsset(assets, AssetType_Texture, filename);
	asset->texture.isFileAlphaPremultiplied = isAlphaPremultiplied;

	return asset;
}

Asset *addSpriteGroup(AssetManager *assets, String name, s32 spriteCount)
{
	ASSERT(spriteCount > 0, "Must have a positive number of sprites in a Sprite Group!");

	Asset *spriteGroup = addAsset(assets, AssetType_Sprite, name, false);
	spriteGroup->data = allocate(assets, spriteCount * sizeof(Sprite));
	spriteGroup->spriteGroup.count = spriteCount;
	spriteGroup->spriteGroup.sprites = (Sprite*) spriteGroup->data.memory;

	return spriteGroup;
}

Sprite *addSprite(AssetManager *assets, String name, Asset *textureAsset, Rect2 uv)
{
	ASSERT(textureAsset != null, "Attempted to add a sprite with no Texture!");

	Asset *spriteGroup = addSpriteGroup(assets, name, 1);

	Sprite *sprite = spriteGroup->spriteGroup.sprites + 0;
	sprite->texture = textureAsset;
	sprite->uv = uv;

	return sprite;
}

void addTiledSprites(AssetManager *assets, String name, String textureFilename, u32 tileWidth, u32 tileHeight, u32 tilesAcross, u32 tilesDown, bool isAlphaPremultiplied=false)
{
	String textureName = pushString(&assets->assetArena, textureFilename);
	Asset **findResult = find(&assets->assetsByName[AssetType_Texture], textureName);
	Asset *textureAsset;
	if (findResult == null)
	{
		textureAsset = addTexture(assets, textureName, isAlphaPremultiplied);
	}
	else
	{
		textureAsset = *findResult;
	}

	ASSERT(textureAsset != null, "Failed to find/create texture for Sprite!");

	Asset *spriteGroup = addSpriteGroup(assets, name, tilesAcross * tilesDown);
	Rect2 uv = rectXYWH(0, 0, (f32)tileWidth, (f32)tileHeight);

	s32 spriteIndex = 0;
	for (u32 y = 0; y < tilesDown; y++)
	{
		uv.y = (f32)(y * tileHeight);

		for (u32 x = 0; x < tilesAcross; x++)
		{
			uv.x = (f32)(x * tileWidth);

			Sprite *sprite = spriteGroup->spriteGroup.sprites + spriteIndex++;
			sprite->texture = textureAsset;
			sprite->uv = uv;
		}
	}
}

void addFont(AssetManager *assets, String name, String filename)
{
	addAsset(assets, AssetType_BitmapFont, filename);
	put(&assets->theme.fontNamesToAssetNames, name, filename);
}

void addShader(AssetManager *assets, ShaderType shaderID, char *name, char *vertFilename, char *fragFilename)
{
	Asset *asset = addAsset(assets, AssetType_Shader, pushString(&assets->assetArena, name), false);
	asset->shader.shaderType = shaderID;
	asset->shader.vertexShaderFilename   = pushString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, vertFilename));
	asset->shader.fragmentShaderFilename = pushString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, fragFilename));
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

	assets->assetReloadHasJustHappened = true;
}

	// NB: TEMPORARY, Windows-specific filesystem reading code!!!!!!!
	// NB: TEMPORARY, Windows-specific filesystem reading code!!!!!!!
	// NB: TEMPORARY, Windows-specific filesystem reading code!!!!!!!
	// NB: TEMPORARY, Windows-specific filesystem reading code!!!!!!!
	// NB: TEMPORARY, Windows-specific filesystem reading code!!!!!!!
void addAssetsFromDirectory(AssetManager *assets, String subDirectory, AssetType manualAssetType=AssetType_Unknown)
{
	String pathToScan;
	if (subDirectory.length == 0)
	{
		pathToScan = myprintf("{0}\\*", {assets->assetsPath}, true);
	}
	else
	{
		pathToScan = myprintf("{0}\\{1}\\*", {assets->assetsPath, subDirectory}, true);
	}

	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile;

	hFindFile = FindFirstFile(pathToScan.chars, &findFileData);
	if (hFindFile == INVALID_HANDLE_VALUE)
	{
		logError("Failed to read directory listing '{0}' (error {1})", {pathToScan, formatInt((u32)GetLastError())});
	}
	else
	{
		do
		{
			if ((findFileData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY))
				|| (findFileData.cFileName[0] == '.'))
			{
				 continue;
			}

			String filename = pushString(&assets->assetArena, findFileData.cFileName);
			AssetType assetType = manualAssetType;

			// Attempt to categorise the asset based on file extension
			if (assetType == AssetType_Unknown)
			{
				String fileExtension = getFileExtension(filename);
				AssetType *foundAssetType = find(&assets->fileExtensionToType, fileExtension);
				assetType = (foundAssetType == null) ? AssetType_Misc : *foundAssetType;
				logInfo("Found asset file '{0}'. Adding as type {1}, calculated from extension '{2}'", {filename, formatInt(assetType), fileExtension});
			}
			else
			{
				logInfo("Found asset file '{0}'. Adding as type {1}, passed in.", {filename, formatInt(assetType)});
			}

			addAsset(assets, assetType, filename);
		}
		while (FindNextFile(hFindFile, &findFileData));
		FindClose(hFindFile);
	}
}

void addAssets(AssetManager *assets)
{
	{
		DEBUG_BLOCK("Read asset directories");
		addAssetsFromDirectory(assets, nullString);

		for (auto it = iterate(&assets->directoryNameToType);
			!it.isDone;
			next(&it))
		{
			auto entry = getEntry(it);
			addAssetsFromDirectory(assets, entry->key, entry->value);
		}
	}

	// TODO: Settings?

	addShader(assets, Shader_Textured,   "textured",   "textured.vert.glsl",   "textured.frag.glsl");
	addShader(assets, Shader_Untextured, "untextured", "untextured.vert.glsl", "untextured.frag.glsl");
	addShader(assets, Shader_PixelArt,   "pixelart",   "pixelart.vert.glsl",   "pixelart.frag.glsl");
}

void reloadAssets(AssetManager *assets, Renderer *renderer, UIState *uiState)
{
	DEBUG_FUNCTION();
	// Preparation
	consoleWriteLine("Reloading assets...");
	renderer->unloadAssets(renderer, assets);
	SDL_Cursor *systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	SDL_SetCursor(systemWaitCursor);
	defer { SDL_FreeCursor(systemWaitCursor); };

	// Actual reloading

	// Clear managed assets
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		unloadAsset(assets, asset);
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