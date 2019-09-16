#pragma once

void initAssets()
{
	bootstrapArena(Assets, assets, assetArena);

	ASSERT(assets->assetArena.currentBlock != null); //initAssetManager() called with uninitialised/corrupted memory arena!
	char *basePath = SDL_GetBasePath();
	assets->assetsPath = pushString(&assets->assetArena, constructPath({makeString(basePath), "assets"s}));

	// NB: We only need to define these for assets in the root assets/ directory
	// Well, for now at least.
	// - Sam, 19/05/2019
	initHashTable(&assets->fileExtensionToType);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "buildings"), AssetType_BuildingDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "cursors"),   AssetType_CursorDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "palettes"),  AssetType_PaletteDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "terrain"),   AssetType_TerrainDefs);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "keymap"),    AssetType_DevKeymap);
	put(&assets->fileExtensionToType, pushString(&assets->assetArena, "theme"),     AssetType_UITheme);

	initHashTable(&assets->directoryNameToType);
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

Blob assetsAllocate(Assets *theAssets, smm size)
{
	Blob result = {};
	result.size = size;
	result.memory = allocateRaw(size);

	theAssets->assetMemoryAllocated += size;
	theAssets->maxAssetMemoryAllocated = max(theAssets->assetMemoryAllocated, theAssets->maxAssetMemoryAllocated);

	return result;
}

Asset *addAsset(AssetType type, String shortName, u32 flags)
{
	Asset *existing = getAssetIfExists(type, shortName);
	if (existing) return existing;

	Asset *asset = appendBlank(&assets->allAssets);
	asset->type = type;
	asset->shortName = shortName;
	if (flags & Asset_IsAFile)
	{
		asset->fullName = pushString(&assets->assetArena, getAssetPath(asset->type, shortName));
	}
	asset->state = AssetState_Unloaded;
	asset->data.size = 0;
	asset->data.memory = null;
	asset->flags = flags;

	put(&assets->assetsByType[type], shortName, asset);

	return asset;
}

void copyFileIntoAsset(Blob *fileData, Asset *asset)
{
	asset->data = assetsAllocate(assets, fileData->size);
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

void ensureAssetIsLoaded(Asset *asset)
{
	if (asset->state == AssetState_Loaded) return;

	loadAsset(asset);

	if (asset->state != AssetState_Loaded)
	{
		logError("Failed to load asset '{0}'", {asset->shortName});
	}
}

void loadTexts(HashTable<String> *texts, Asset *asset, Blob fileData)
{
	LineReader reader = readLines(asset->shortName, fileData);

	clear(texts);

	while (loadNextLine(&reader))
	{
		String key = readToken(&reader);
		String value = getRemainderOfLine(&reader);

		put(texts, key, value);
	}
}

void loadAsset(Asset *asset)
{
	DEBUG_FUNCTION();
	if (asset->state != AssetState_Unloaded) return;

	String assetFileName = asset->fullName;
	if (asset->flags & Asset_IsLocaleSpecific)
	{
		assetFileName = myprintf(asset->fullName, {getLocale()}, true);
	}

	Blob fileData = {};
	// Some assets (meta-assets?) have no file associated with them, because they are composed of other assets.
	// eg, ShaderPrograms are made of several ShaderParts.
	if (asset->flags & Asset_IsAFile)
	{
		fileData = readTempFile(assetFileName);
	}

	// Type-specific loading
	switch (asset->type)
	{
		case AssetType_BitmapFont:
		{
			loadBMFont(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_BuildingDefs:
		{
			loadBuildingDefs(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Cursor:
		{
			fileData = readTempFile(asset->cursor.imageFilePath);
			SDL_Surface *cursorSurface = createSurfaceFromFileData(fileData, asset->shortName);
			asset->cursor.sdlCursor = SDL_CreateColorCursor(cursorSurface, asset->cursor.hotspot.x, asset->cursor.hotspot.y);
			SDL_FreeSurface(cursorSurface);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_CursorDefs:
		{
			loadCursorDefs(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_DevKeymap:
		{
			if (globalConsole != null)
			{
				// NB: We keep the keymap file in the asset memory, so that the CommandShortcut.command can
				// directly refer to the string data from the file, instead of having to assetsAllocate a copy
				// and not be able to free it ever. This is more memory efficient.
				copyFileIntoAsset(&fileData, asset);
				loadConsoleKeyboardShortcuts(globalConsole, fileData, asset->shortName);
			}
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Palette:
		{
			Palette *palette = &asset->palette;
			switch (palette->type)
			{
				case PaletteType_Gradient: {
					asset->data = assetsAllocate(assets, palette->size * sizeof(V4));
					palette->paletteData = makeArray<V4>(palette->size, (V4*)asset->data.memory);
					
					f32 ratio = 1.0f / (f32)(palette->size);
					for (s32 i=0; i < palette->size; i++)
					{
						palette->paletteData[i] = lerp(palette->gradient.from, palette->gradient.to, i * ratio);
					}
				} break;

				case PaletteType_Fixed: {} break;

				INVALID_DEFAULT_CASE;
			}
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_PaletteDefs:
		{
			loadPaletteDefs(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Shader:
		{
			copyFileIntoAsset(&fileData, asset);
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
				ensureAssetIsLoaded(t);
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
			loadTerrainDefs(&terrainDefs, fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texts:
		{
			fileData = readTempFile(getAssetPath(AssetType_Texts, myprintf("{0}.text", {getLocale()})));
			copyFileIntoAsset(&fileData, asset);
			loadTexts(&assets->texts, asset, fileData);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texture:
		{
			SDL_Surface *surface = createSurfaceFromFileData(fileData, assetFileName);
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
			loadUITheme(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		default:
		{
			if (asset->flags & Asset_IsAFile)
			{
				copyFileIntoAsset(&fileData, asset);
			}

			asset->state = AssetState_Loaded;
		} break;
	}
}

void unloadAsset(Asset *asset)
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
		deallocateRaw(asset->data.memory);
	}

	asset->state = AssetState_Unloaded;
}

Asset *addTexture(String filename, bool isAlphaPremultiplied)
{
	Asset *asset = addAsset(AssetType_Texture, filename);
	asset->texture.isFileAlphaPremultiplied = isAlphaPremultiplied;

	return asset;
}

Asset *addSpriteGroup(String name, s32 spriteCount)
{
	ASSERT(spriteCount > 0); //Must have a positive number of sprites in a Sprite Group!

	Asset *spriteGroup = addAsset(AssetType_Sprite, name, 0);
	if (spriteGroup->data.size != 0) DEBUG_BREAK(); // @Leak! Creating the sprite group multiple times is probably a bad idea for other reasons too.
	spriteGroup->data = assetsAllocate(assets, spriteCount * sizeof(Sprite));
	spriteGroup->spriteGroup.count = spriteCount;
	spriteGroup->spriteGroup.sprites = (Sprite*) spriteGroup->data.memory;

	return spriteGroup;
}

void addTiledSprites(String name, String textureFilename, u32 tileWidth, u32 tileHeight, u32 tilesAcross, u32 tilesDown, bool isAlphaPremultiplied)
{
	String textureName = pushString(&assets->assetArena, textureFilename);
	Asset **findResult = find(&assets->assetsByType[AssetType_Texture], textureName);
	Asset *textureAsset;
	if (findResult == null)
	{
		textureAsset = addTexture(textureName, isAlphaPremultiplied);
	}
	else
	{
		textureAsset = *findResult;
	}

	ASSERT(textureAsset != null); //Failed to find/create texture for Sprite!

	Asset *spriteGroup = addSpriteGroup(name, tilesAcross * tilesDown);
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

void addTiledSprites(String name, Asset *texture, V2I tileSize, V2I tileBorder, Rect2I selectedTiles)
{
	ASSERT(texture->type == AssetType_Texture);

	Asset *spriteGroup = addSpriteGroup(name, selectedTiles.w * selectedTiles.h);

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

void addFont(String name, String filename)
{
	addAsset(AssetType_BitmapFont, filename);
	put(&assets->theme.fontNamesToAssetNames, name, filename);
}

void loadAssets()
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		loadAsset(asset);
	}

	assets->assetReloadHasJustHappened = true;
}

void addAssetsFromDirectory(String subDirectory, AssetType manualAssetType)
{
	String pathToScan;
	if (isEmpty(subDirectory))
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
			|| (fileInfo->filename[0] == '.'))
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
			// logInfo("Found asset file '{0}'. Adding as type {1}, calculated from extension '{2}'", {filename, formatInt(assetType), fileExtension});
		}
		else
		{
			// logInfo("Found asset file '{0}'. Adding as type {1}, passed in.", {filename, formatInt(assetType)});
		}

		addAsset(assetType, filename);
	}
}

void addAssets()
{
	// Manually add the texts asset, because it's special.
	addAsset(AssetType_Texts, "{0}.text"s, AssetDefaultFlags | Asset_IsLocaleSpecific);

	{
		DEBUG_BLOCK("Read asset directories");
		addAssetsFromDirectory(nullString);

		for (auto it = iterate(&assets->directoryNameToType);
			!it.isDone;
			next(&it))
		{
			auto entry = getEntry(it);
			addAssetsFromDirectory(entry->key, entry->value);
		}
	}
}

bool haveAssetFilesChanged()
{
	return hasDirectoryChanged(&assets->assetChangeHandle);
}

void reloadAssets()
{
	DEBUG_FUNCTION();

	// Preparation
	consoleWriteLine("Reloading assets...");
	rendererUnloadAssets();

	// Clear managed assets
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		unloadAsset(asset);
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
	addAssets();
	loadAssets();

	// After stuff
	rendererLoadAssets();
	consoleWriteLine("Assets reloaded successfully!", CLS_Success);
}

void reloadAsset(Asset *asset)
{
	unloadAsset(asset);
	loadAsset(asset);
}

Asset *getAssetIfExists(AssetType type, String shortName)
{
	Asset **result = find(&assets->assetsByType[type], shortName);

	return (result == null) ? null : *result;
}

Asset *getAsset(AssetType type, String shortName)
{
	DEBUG_FUNCTION();
	Asset *result = getAssetIfExists(type, shortName);

	if (result == null)
	{
		DEBUG_BREAK();
		logError("Requested asset '{0}' was not found!", {shortName});
	}
	
	return result;
}

inline Array<V4> *getPalette(String name)
{
	return &getAsset(AssetType_Palette, name)->palette.paletteData;
}

inline SpriteGroup *getSpriteGroup(String name)
{
	return &getAsset(AssetType_Sprite, name)->spriteGroup;
}

inline Sprite *getSprite(SpriteGroup *group, s32 offset)
{
	return group->sprites + (offset % group->count);
}

inline Shader *getShader(String shaderName)
{
	return &getAsset(AssetType_Shader, shaderName)->shader;
}

BitmapFont *getFont(String fontName)
{
	BitmapFont *result = null;

	String *fontFilename = find(&assets->theme.fontNamesToAssetNames, fontName);
	if (fontFilename == null)
	{
		// Fall back to treating it as a filename
		Asset *fontAsset = getAsset(AssetType_BitmapFont, fontName);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
		logError("Failed to find font named '{0}' in the UITheme.", {fontName});
	}
	else
	{
		Asset *fontAsset = getAsset(AssetType_BitmapFont, *fontFilename);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
	}

	return result;
}

inline String getText(String name)
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
		logWarn("Locale {0} is missing text for '{1}'.", {getLocale(), name});
		put(&assets->texts, name, name);
	}

	return result;
}

String getAssetPath(AssetType type, String shortName)
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

void reloadLocaleSpecificAssets()
{
	for (auto it = iterate(&assets->allAssets); !it.isDone; next(&it))
	{
		Asset *asset = get(it);
		if (asset->flags & Asset_IsLocaleSpecific)
		{
			reloadAsset(asset);
		}
	}

	assets->assetReloadHasJustHappened = true;
}

void loadCursorDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	while (loadNextLine(&reader))
	{
		String name     = pushString(&assets->assetArena, readToken(&reader));
		String filename = readToken(&reader);

		Maybe<s64> hotX = readInt(&reader);
		Maybe<s64> hotY = readInt(&reader);

		if (hotX.isValid && hotY.isValid)
		{
			// Add the cursor
			Asset *cursorAsset = addAsset(AssetType_Cursor, name, 0);
			cursorAsset->cursor.imageFilePath = pushString(&assets->assetArena, getAssetPath(AssetType_Cursor, filename));
			cursorAsset->cursor.hotspot = v2i(truncate32(hotX.value), truncate32(hotY.value));
		}
		else
		{
			error(&reader, "Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'.");
			return;
		}
	}
}

void loadPaletteDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	Asset *paletteAsset = null;

	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);
		if (command[0] == ':')
		{
			command.length--;
			command.chars++;

			if (equals(command, "Palette"s))
			{
				String paletteName = pushString(&assets->assetArena, readToken(&reader));
				paletteAsset = addAsset(AssetType_Palette, paletteName, 0);
			}
			else
			{
				error(&reader, "Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!", {command});
				return;
			}
		}
		else
		{
			if (paletteAsset == null)
			{
				error(&reader, "Unexpected command '{0}' before the start of a :Palette", {command});
				return;
			}

			if (equals(command, "type"s))
			{
				String type = readToken(&reader);

				if (equals(type, "fixed"s))
				{
					paletteAsset->palette.type = PaletteType_Fixed;
				}
				else if (equals(type, "gradient"s))
				{
					paletteAsset->palette.type = PaletteType_Gradient;
				}
				else
				{
					error(&reader, "Unrecognised palette type '{0}', allowed values are: fixed, gradient", {type});
				}
			}
			else if (equals(command, "size"s))
			{
				Maybe<s64> size = readInt(&reader);
				if (size.isValid)
				{
					paletteAsset->palette.size = (s32)size.value;
				}
			}
			else if (equals(command, "color"s))
			{
				Maybe<V4> color = readColor(&reader);
				if (color.isValid)
				{
					if (paletteAsset->palette.type == PaletteType_Fixed)
					{
						if (!isInitialised(&paletteAsset->palette.paletteData))
						{
							paletteAsset->data = assetsAllocate(assets, paletteAsset->palette.size * sizeof(V4));
							paletteAsset->palette.paletteData = makeArray<V4>(paletteAsset->palette.size, (V4*)paletteAsset->data.memory);
						}

						s32 colorIndex = paletteAsset->palette.fixed.currentPos++;
						if (colorIndex >= paletteAsset->palette.size)
						{
							error(&reader, "Too many 'color' definitions! 'size' must be large enough.");
						}
						else
						{
							paletteAsset->palette.paletteData[colorIndex] = color.value;
						}
					}
					else
					{
						error(&reader, "'color' is only a valid command for fixed palettes.");
					}
				}
			}
			else if (equals(command, "from"s))
			{
				Maybe<V4> from = readColor(&reader);
				if (from.isValid)
				{
					if (paletteAsset->palette.type == PaletteType_Gradient)
					{ 
						paletteAsset->palette.gradient.from = from.value;
					}
					else
					{
						error(&reader, "'from' is only a valid command for gradient palettes.");
					}
				}
			}
			else if (equals(command, "to"s))
			{
				Maybe<V4> to = readColor(&reader);
				if (to.isValid)
				{
					if (paletteAsset->palette.type == PaletteType_Gradient)
					{ 
						paletteAsset->palette.gradient.to = to.value;
					}
					else
					{
						error(&reader, "'to' is only a valid command for gradient palettes.");
					}
				}
			}
			else
			{
				error(&reader, "Unrecognised command '{0}'", {command});
			}
		}
	}
}
