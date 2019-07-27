#pragma once

void initAssetManager(AssetManager *assets)
{
	ASSERT(assets->assetArena.currentBlock != null); //initAssetManager() called with uninitialised/corrupted memory arena!
	char *basePath = SDL_GetBasePath();
	assets->assetsPath = pushString(&assets->assetArena, constructPath({makeString(basePath), makeString("assets")}));

	// NB: We only need to define these for assets in the root assets/ directory
	// Well, for now at least.
	// - Sam, 19/05/2019
	initHashTable(&assets->fileExtensionToType);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "buildings"), AssetType_BuildingDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "terrain"),   AssetType_TerrainDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "keymap"),    AssetType_DevKeymap);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "theme"),     AssetType_UITheme);

	initHashTable(&assets->directoryNameToType);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "cursors"),   AssetType_Cursor);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "fonts"),     AssetType_BitmapFont);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "shaders"),   AssetType_Shader);
	put(&assets->directoryNameToType, pushString(&assets->assetArena, "textures"),  AssetType_Texture);


	// NB: This has to happen just after the last addition to the AssetArena which should remain
	// across asset reloads. Maybe we should just have multiple arenas? IDK.
	markResetPosition(&assets->assetArena);

	initChunkedArray(&assets->allAssets, &assets->assetArena, 2048);
	assets->assetMemoryAllocated = 0;
	assets->maxAssetMemoryAllocated = 0;

	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		initHashTable(&assets->assetsByType[assetType]);
	}

	initUITheme(&assets->theme);

	initHashTable(&assets->texts);

	// NB: This might fail, or we might be on a platform where it isn't implemented.
	// That's OK though! 
	assets->assetChangeHandle = beginWatchingDirectory(assets->assetsPath);
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

Asset *addAsset(AssetManager *assets, AssetType type, String shortName, bool isAFile)
{
	Asset *existing = getAssetIfExists(assets, type, shortName);
	if (existing) return existing;

	Asset *asset = appendBlank(&assets->allAssets);
	asset->type = type;
	if (shortName.length != 0)
	{
		asset->shortName = shortName;
		asset->fullName = pushString(&assets->assetArena, getAssetPath(assets, asset->type, shortName));
	}
	asset->state = AssetState_Unloaded;
	asset->data.size = 0;
	asset->data.memory = null;
	asset->isAFile = isAFile;

	put(&assets->assetsByType[type], shortName, asset);

	return asset;
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

	ASSERT(fileData.size > 0);//, "Attempted to create a surface from an unloaded asset! ({0})", {name});
	ASSERT(fileData.size < s32Max);//, "File '{0}' is too big for SDL's RWOps!", {name});

	SDL_RWops *rw = SDL_RWFromConstMem(fileData.memory, truncate32(fileData.size));
	if (rw)
	{
		result = IMG_Load_RW(rw, 0);

		if (result == null)
		{
			logError("Failed to create SDL_Surface from asset '{0}'!\n{1}", {
				name, makeString(IMG_GetError())
			});
		}

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

	if (asset->state != AssetState_Loaded)
	{
		logError("Failed to load asset '{0}'", {asset->shortName});
	}
}

void loadTexts(HashTable<String> *texts, Asset *asset, Blob fileData)
{
	LineReader reader = readLines(asset->shortName, fileData);

	clear(texts);

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);

		String key, value;
		key = nextToken(line, &value);

		put(texts, key, value);
	}
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
			loadBuildingDefs(assets, fileData, asset);
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
			copyFileIntoAsset(assets, &fileData, asset);
			splitInTwo(stringFromBlob(fileData), '$', &asset->shader.vertexShader, &asset->shader.fragmentShader);
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

		case AssetType_Texts:
		{
			fileData = readTempFile(getAssetPath(assets, AssetType_Texts, myprintf("{0}.text", {assets->locale})));
			copyFileIntoAsset(assets, &fileData, asset);
			loadTexts(&assets->texts, asset, fileData);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texture:
		{
			SDL_Surface *surface = createSurfaceFromFileData(fileData, asset->fullName);
			ASSERT(surface->format->BytesPerPixel == 4); //We only handle 32-bit colour images!

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
	ASSERT(spriteCount > 0); //Must have a positive number of sprites in a Sprite Group!

	Asset *spriteGroup = addAsset(assets, AssetType_Sprite, name, false);
	spriteGroup->data = allocate(assets, spriteCount * sizeof(Sprite));
	spriteGroup->spriteGroup.count = spriteCount;
	spriteGroup->spriteGroup.sprites = (Sprite*) spriteGroup->data.memory;

	return spriteGroup;
}

void addTiledSprites(AssetManager *assets, String name, String textureFilename, u32 tileWidth, u32 tileHeight, u32 tilesAcross, u32 tilesDown, bool isAlphaPremultiplied)
{
	String textureName = pushString(&assets->assetArena, textureFilename);
	Asset **findResult = find(&assets->assetsByType[AssetType_Texture], textureName);
	Asset *textureAsset;
	if (findResult == null)
	{
		textureAsset = addTexture(assets, textureName, isAlphaPremultiplied);
	}
	else
	{
		textureAsset = *findResult;
	}

	ASSERT(textureAsset != null); //Failed to find/create texture for Sprite!

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

void addTiledSprites(AssetManager *assets, String name, Asset *texture, V2I tileSize, V2I tileBorder, Rect2I selectedTiles)
{
	ASSERT(texture->type == AssetType_Texture);

	Asset *spriteGroup = addSpriteGroup(assets, name, selectedTiles.w * selectedTiles.h);

	s32 strideX = tileSize.x + (tileBorder.x * 2);
	s32 strideY = tileSize.y + (tileBorder.y * 2);

	s32 spriteIndex = 0;
	for (s32 y = selectedTiles.y; y < selectedTiles.y + selectedTiles.h; y++)
	{
		for (s32 x = selectedTiles.x; x < selectedTiles.x + selectedTiles.w; x++)
		{
			Sprite *sprite = spriteGroup->spriteGroup.sprites + spriteIndex++;
			sprite->texture = texture;
			sprite->uv = rectXYWHi(
				(x * strideX) + tileBorder.x,
				(y * strideY) + tileBorder.y,
				tileSize.x, tileSize.y
			);
		}
	}
}

void addFont(AssetManager *assets, String name, String filename)
{
	addAsset(assets, AssetType_BitmapFont, filename);
	put(&assets->theme.fontNamesToAssetNames, name, filename);
}

void loadAssets(AssetManager *assets)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		loadAsset(assets, asset);
	}

	assets->assetReloadHasJustHappened = true;
}

void addAssetsFromDirectory(AssetManager *assets, String subDirectory, AssetType manualAssetType)
{
	String pathToScan;
	if (subDirectory.length == 0)
	{
		pathToScan = constructPath({assets->assetsPath}, true);
	}
	else
	{
		pathToScan = constructPath({assets->assetsPath, subDirectory}, true);
	}

	for (auto it = iterateDirectoryListing(pathToScan);
		hasNextFile(&it);
		findNextFile(&it))
	{
		FileInfo *fileInfo = getFileInfo(&it);

		if ((fileInfo->flags & (FileFlag_Hidden | FileFlag_Directory))
			|| (fileInfo->filename.chars[0] == '.'))
		{
			 continue;
		}

		String filename = pushString(&assets->assetArena, fileInfo->filename);
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
}

void addAssets(AssetManager *assets)
{
	// Manually add the texts asset, because it's special.
	assets->locale = globalAppState.settings.locale;
	addAsset(assets, AssetType_Texts, makeString("LOCALE.text"), false);

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
}

bool haveAssetFilesChanged(AssetManager *assets)
{
	return hasDirectoryChanged(&assets->assetChangeHandle);
}

void reloadAssets(AssetManager *assets, Renderer *renderer, UIState *uiState)
{
	DEBUG_FUNCTION();

	// Preparation
	consoleWriteLine("Reloading assets...");
	rendererUnloadAssets(renderer, assets);
	SDL_Cursor *systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	SDL_SetCursor(systemWaitCursor);
	defer { SDL_FreeCursor(systemWaitCursor); };

	// Clear managed assets
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		unloadAsset(assets, asset);
	}

	// Clear the hash tables
	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		clear(&assets->assetsByType[assetType]);
	}

	// General resetting of Assets system
	// The "throw everything away and start over" method of reloading. It's dumb but effective!
	// We're now not *quite* throwing everything away.
	resetMemoryArena(&assets->assetArena);
	initChunkedArray(&assets->allAssets, &assets->assetArena, 2048);
	addAssets(assets);
	loadAssets(assets);

	// After stuff
	setCursor(uiState, uiState->currentCursor);
	rendererLoadAssets(renderer, assets);
	consoleWriteLine("Assets reloaded successfully!", CLS_Success);
}

void reloadAsset(AssetManager *assets, Asset *asset)
{
	unloadAsset(assets, asset);
	loadAsset(assets, asset);
}

Asset *getAssetIfExists(AssetManager *assets, AssetType type, String shortName)
{
	Asset **result = find(&assets->assetsByType[type], shortName);

	return (result == null) ? null : *result;
}

Asset *getAsset(AssetManager *assets, AssetType type, String shortName)
{
	DEBUG_FUNCTION();
	Asset *result = getAssetIfExists(assets, type, shortName);

	if (result == null)
	{
		DEBUG_BREAK();
		logError("Requested asset '{0}' was not found!", {shortName});
	}

	return result;
}

inline SpriteGroup *getSpriteGroup(AssetManager *assets, String name)
{
	return &getAsset(assets, AssetType_Sprite, name)->spriteGroup;
}

inline Sprite *getSprite(SpriteGroup *group, s32 offset)
{
	return group->sprites + (offset % group->count);
}

inline Shader *getShader(AssetManager *assets, String shaderName)
{
	return &getAsset(assets, AssetType_Shader, shaderName)->shader;
}

BitmapFont *getFont(AssetManager *assets, String fontName)
{
	BitmapFont *result = null;

	String *fontFilename = find(&assets->theme.fontNamesToAssetNames, fontName);
	if (fontFilename == null)
	{
		// Fall back to treating it as a filename
		Asset *fontAsset = getAsset(assets, AssetType_BitmapFont, fontName);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
		logError("Failed to find font named '{0}' in the UITheme.", {fontName});
	}
	else
	{
		Asset *fontAsset = getAsset(assets, AssetType_BitmapFont, *fontFilename);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
	}

	return result;
}

inline String getText(AssetManager *assets, String name)
{
	DEBUG_FUNCTION();

	String result = name;

	String *foundText = find(&assets->texts, name);
	if (foundText != null)
	{
		result = *foundText;
	}
	else
	{
		logError("Locale {0} is missing text for '{1}'.", {globalAppState.settings.locale, name});
		put(&assets->texts, name, name);
	}

	return result;
}

String getAssetPath(AssetManager *assets, AssetType type, String shortName)
{
	String result = shortName;

	switch (type)
	{
	case AssetType_Cursor:
		result = myprintf("{0}/cursors/{1}", {assets->assetsPath, shortName}, true);
		break;
	case AssetType_BitmapFont:
		result = myprintf("{0}/fonts/{1}",    {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Shader:
		result = myprintf("{0}/shaders/{1}",  {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texts:
		result = myprintf("{0}/locale/{1}", {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texture:
		result = myprintf("{0}/textures/{1}", {assets->assetsPath, shortName}, true);
		break;
	default:
		result = myprintf("{0}/{1}", {assets->assetsPath, shortName}, true);
		break;
	}

	return result;
}

void setLocale(AssetManager *assets, String locale)
{
	if (!equals(locale, assets->locale))
	{
		assets->locale = locale;

		// Text
		Asset *textAsset = getAsset(assets, AssetType_Texts, makeString("LOCALE.text"));
		reloadAsset(assets, textAsset);
	}
}
