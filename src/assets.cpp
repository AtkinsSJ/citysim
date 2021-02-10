#pragma once

void initAssets()
{
	bootstrapArena(Assets, assets, assetArena);

	initStringTable(&assets->assetStrings);

	ASSERT(assets->assetArena.currentBlock != null); //initAssetManager() called with uninitialised/corrupted memory arena!
	String basePath = makeString(SDL_GetBasePath());
	assets->assetsPath = intern(&assets->assetStrings, constructPath({basePath, "assets"_s}));

	// NB: We only need to define these for assets in the root assets/ directory
	// Well, for now at least.
	// - Sam, 19/05/2019
	initHashTable(&assets->fileExtensionToType);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "buildings"_s), AssetType_BuildingDefs);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "cursors"_s),   AssetType_CursorDefs);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "keymap"_s),    AssetType_DevKeymap);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "palettes"_s),  AssetType_PaletteDefs);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "sprites"_s),   AssetType_SpriteDefs);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "terrain"_s),   AssetType_TerrainDefs);
	put(&assets->fileExtensionToType, intern(&assets->assetStrings, "theme"_s),     AssetType_UITheme);

	initHashTable(&assets->directoryNameToType);
	put(&assets->directoryNameToType, intern(&assets->assetStrings, "fonts"_s),     AssetType_BitmapFont);
	put(&assets->directoryNameToType, intern(&assets->assetStrings, "shaders"_s),   AssetType_Shader);
	put(&assets->directoryNameToType, intern(&assets->assetStrings, "textures"_s),  AssetType_Texture);
	put(&assets->directoryNameToType, intern(&assets->assetStrings, "locale"_s),    AssetType_Texts);

	initChunkedArray(&assets->allAssets, &assets->assetArena, 2048);
	assets->assetMemoryAllocated = 0;
	assets->maxAssetMemoryAllocated = 0;

	auto compareStrings = [](String *a, String *b) {return equals(*a, *b); };

	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		initHashTable(&assets->assetsByType[assetType]);

		initSet<String>(&assets->missingAssetNames[assetType], &assets->assetArena, compareStrings);
	}

	initUITheme(&assets->theme);

	initTerrainCatalogue();
	initBuildingCatalogue();

	initHashTable(&assets->texts);
	initHashTable(&assets->defaultTexts);

	initSet<String>(&assets->missingTextIDs, &assets->assetArena, compareStrings);

	// Placeholder assets!
	{
		// Texture
		Asset *placeholderTexture = &assets->placeholderAssets[AssetType_Texture];
		*placeholderTexture = {};
		placeholderTexture->type = AssetType_Texture;
		placeholderTexture->state = AssetState_Loaded;
		placeholderTexture->data = assetsAllocate(assets, 2 * 2 * sizeof(u32));
		u32 *pixels = (u32*)placeholderTexture->data.memory;
		pixels[0] = pixels[3] = 0xffff00ff;
		pixels[1] = pixels[2] = 0xff000000;
		placeholderTexture->texture.surface = SDL_CreateRGBSurfaceFrom(pixels, 2, 2, 32, 2*sizeof(u32),
							0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		placeholderTexture->texture.isFileAlphaPremultiplied = true;

		// Sprite!
		Asset *placeholderSprite = &assets->placeholderAssets[AssetType_Sprite];
		*placeholderSprite = {};
		placeholderSprite->type = AssetType_Sprite;
		placeholderSprite->state = AssetState_Loaded;
		placeholderSprite->data = assetsAllocate(assets, 1 * sizeof(Sprite));
		placeholderSprite->spriteGroup.count = 1;
		placeholderSprite->spriteGroup.sprites = (Sprite*) placeholderSprite->data.memory;
		placeholderSprite->spriteGroup.sprites[0].texture = &assets->placeholderAssets[AssetType_Texture];
		placeholderSprite->spriteGroup.sprites[0].uv = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);

		// Cursor
		Asset *placeholderCursor = &assets->placeholderAssets[AssetType_Cursor];
		*placeholderCursor = {};
		placeholderCursor->type = AssetType_Cursor;
		placeholderCursor->state = AssetState_Loaded;
		placeholderCursor->cursor.sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

		// Palette
		Asset *placeholderPalette = &assets->placeholderAssets[AssetType_Palette];
		*placeholderPalette = {};
		placeholderPalette->type = AssetType_Palette;
		placeholderPalette->state = AssetState_Loaded;
		placeholderPalette->palette.type = PaletteType_Fixed;
		placeholderPalette->palette.size = 0;
		placeholderPalette->palette.paletteData = makeArray<V4>(0, null);
	}

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
	String internedShortName = intern(&assets->assetStrings, shortName);

	Asset *existing = getAssetIfExists(type, internedShortName);
	if (existing) return existing;

	Asset *asset = appendBlank(&assets->allAssets);
	asset->type = type;
	asset->shortName = internedShortName;
	if (flags & Asset_IsAFile)
	{
		asset->fullName = intern(&assets->assetStrings, getAssetPath(asset->type, internedShortName));
	}
	asset->state = AssetState_Unloaded;
	asset->data.size = 0;
	asset->data.memory = null;
	asset->flags = flags;

	put(&assets->assetsByType[type], internedShortName, asset);

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
			logError("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, {
				name, makeString(IMG_GetError())
			});
		}

		SDL_RWclose(rw);
	}
	else
	{
		logError("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, {
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
		logError("Failed to load asset '{0}'"_s, {asset->shortName});
	}
}

void loadAsset(Asset *asset)
{
	DEBUG_FUNCTION();
	if (asset->state != AssetState_Unloaded) return;

	if (asset->flags & Asset_IsLocaleSpecific)
	{
		// Only load assets that match our locale
		String assetLocale = getFileLocale(asset->fullName);

		if (equals(assetLocale, getLocale()))
		{
			asset->texts.isFallbackLocale = false;
		}
		else
		{
			if (equals(assetLocale, "en"_s))
			{
				logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, {asset->fullName, assetLocale, getLocale()});
				asset->texts.isFallbackLocale = true;
			}
			else
			{
				logInfo("Skipping asset {0} because it's the wrong locale. ({1}, current is {2})"_s, {asset->fullName, assetLocale, getLocale()});
				return;
			}
		}
	}

	Blob fileData = {};
	// Some assets (meta-assets?) have no file associated with them, because they are composed of other assets.
	// eg, ShaderPrograms are made of several ShaderParts.
	if (asset->flags & Asset_IsAFile)
	{
		fileData = readTempFile(asset->fullName);
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

		case AssetType_SpriteDefs:
		{
			loadSpriteDefs(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_TerrainDefs:
		{
			loadTerrainDefs(fileData, asset);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texts:
		{
			HashTable<String> *textsTable = (asset->texts.isFallbackLocale ? &assets->defaultTexts : &assets->texts);
			loadTexts(textsTable, asset, fileData);
			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Texture:
		{
			// TODO: Emergency debug texture that's used if loading a file fails.
			// Right now, we just crash! (Not shippable)
			SDL_Surface *surface = createSurfaceFromFileData(fileData, asset->fullName);
			if (surface->format->BytesPerPixel != 4)
			{
				logError("Texture asset '{0}' is not 32bit, which is all we support right now. (BytesPerPixel = {1})"_s, {asset->shortName, formatInt(surface->format->BytesPerPixel)});
				return;
			}

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
		case AssetType_BuildingDefs:
		{
			// Remove all of our terrain defs
			removeBuildingDefs(asset->buildingDefs.buildingIDs);
			asset->buildingDefs.buildingIDs = makeArray<String>(0, null);
		} break;

		case AssetType_CursorDefs:
		{
			// Remove all of our cursor assets
			for (s32 cursorIndex = 0; cursorIndex < asset->cursorDefs.cursorNames.count; cursorIndex++)
			{
				String cursorName = asset->cursorDefs.cursorNames[cursorIndex];
				removeAsset(AssetType_Cursor, cursorName);
			}
			asset->cursorDefs.cursorNames = makeArray<String>(0, null);
		} break;

		case AssetType_Cursor:
		{
			if (asset->cursor.sdlCursor != null)
			{
				SDL_FreeCursor(asset->cursor.sdlCursor);
				asset->cursor.sdlCursor = null;
			}
		} break;

		case AssetType_PaletteDefs:
		{
			// Remove all of our palette assets
			for (s32 paletteIndex = 0; paletteIndex < asset->paletteDefs.paletteNames.count; paletteIndex++)
			{
				String paletteName = asset->paletteDefs.paletteNames[paletteIndex];
				removeAsset(AssetType_Palette, paletteName);
			}
			asset->paletteDefs.paletteNames = makeArray<String>(0, null);
		} break;

		case AssetType_SpriteDefs:
		{
			removeSpriteDefs(asset->spriteDefs.spriteGroupNames);
			asset->spriteDefs.spriteGroupNames = makeArray<String>(0, null);
		} break;

		case AssetType_TerrainDefs:
		{
			// Remove all of our terrain defs
			removeTerrainDefs(asset->terrainDefs.terrainIDs);
			asset->terrainDefs.terrainIDs = makeArray<String>(0, null);
		} break;

		case AssetType_Texts:
		{
			// Remove all of our texts from the table
			HashTable<String> *textsTable = (asset->texts.isFallbackLocale ? &assets->defaultTexts : &assets->texts);
			for (s32 keyIndex = 0; keyIndex < asset->texts.keys.count; keyIndex++)
			{
				String key = asset->texts.keys[keyIndex];
				removeKey(textsTable, key);
			}

			asset->texts.keys = makeArray<String>(0, null);
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

void removeAsset(AssetType type, String name)
{
	Asset *asset = getAssetIfExists(type, name);
	if (asset == null)
	{
		logError("Attempted to remove an asset (name `{0}`, type {1}) which doesn't exist!"_s, {name, formatInt(type)});
	}
	else
	{
		unloadAsset(asset);
		removeKey(&assets->assetsByType[type], name);
	}
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

void addFont(String name, String filename)
{
	Asset *asset = addAsset(AssetType_BitmapFont, filename);
	put(&assets->theme.fontNamesToAssetNames, intern(&assets->assetStrings, name), asset->shortName);
}

void loadAssets()
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&assets->allAssets); hasNext(&it); next(&it))
	{
		Asset *asset = get(&it);
		loadAsset(asset);
	}

	assets->lastAssetReloadTicks = SDL_GetTicks();
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

	bool isLocaleSpecific = equals(subDirectory, "locale"_s);
	u32 assetFlags = AssetDefaultFlags;
	if (isLocaleSpecific) assetFlags |= Asset_IsLocaleSpecific;

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

		String filename = intern(&assets->assetStrings, fileInfo->filename);
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

		
		addAsset(assetType, filename, assetFlags);
	}
}

void addAssets()
{
	DEBUG_FUNCTION();

	addAssetsFromDirectory(nullString);

	for (auto it = iterate(&assets->directoryNameToType);
		hasNext(&it);
		next(&it))
	{
		auto entry = getEntry(&it);
		addAssetsFromDirectory(entry->key, entry->value);
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
	consoleWriteLine("Reloading assets..."_s);
	rendererUnloadAssets();

	// Clear managed assets
	for (auto it = iterate(&assets->allAssets); hasNext(&it); next(&it))
	{
		Asset *asset = get(&it);
		unloadAsset(asset);
	}

	// Clear the hash tables
	for (s32 assetType = 0; assetType < AssetTypeCount; assetType++)
	{
		clear(&assets->assetsByType[assetType]);

		// Reset missing text warnings
		clear(&assets->missingAssetNames[assetType]);
	}

	clear(&assets->missingTextIDs);

	// Regenerate asset catalogue
	clear(&assets->allAssets);
	addAssets();
	loadAssets();

	// After stuff
	rendererLoadAssets();
	refreshBuildingSpriteCache(&buildingCatalogue);
	refreshTerrainSpriteCache(&terrainCatalogue);
	consoleWriteLine("Assets reloaded successfully!"_s, CLS_Success);
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
		if (add(&assets->missingAssetNames[type], shortName))
		{
			logWarn("Requested {0} asset '{1}' was not found! Using placeholder."_s, {assetTypeNames[type], shortName});
		}
		result = &assets->placeholderAssets[type];
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

SpriteRef getSpriteRef(String groupName, s32 spriteIndex)
{
	SpriteRef result = {};

	result.spriteGroupName = groupName;
	result.spriteIndex = spriteIndex;

	// NB: We don't retrieve the sprite now, we just leave the pointerRetrievedTicks value at 0
	// so that it WILL be retrieved the first time we call getSprite().

	return result;
}

Sprite *getSprite(SpriteRef *ref)
{
	if (SDL_TICKS_PASSED(assets->lastAssetReloadTicks, ref->pointerRetrievedTicks))
	{
		SpriteGroup *group = getSpriteGroup(ref->spriteGroupName);
		if (group != null)
		{
			ref->pointer = group->sprites + (ref->spriteIndex % group->count);
		}
		else
		{
			// TODO: Dummy sprite!
			ASSERT(!"Sprite group missing");
		}

		ref->pointerRetrievedTicks = SDL_GetTicks();
	}

	return ref->pointer;
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
		logError("Failed to find font named '{0}' in the UITheme."_s, {fontName});
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

inline BitmapFont *getFont(FontReference *fontRef)
{
	if (fontRef->font == null)
	{
		fontRef->font = getFont(fontRef->fontName);
	}

	return fontRef->font;
}

inline String getText(String name)
{
	DEBUG_FUNCTION();

	hashString(&name);

	String result = name;

	String *foundText = find(&assets->texts, name);
	if (foundText != null)
	{
		result = *foundText;
	}
	else
	{
		// Try to fall back to english if possible
		String *defaultText = find(&assets->defaultTexts, name);
		if (defaultText != null)
		{
			result = *defaultText;
		}

		// Dilemma: We probably want to report missing texts, somehow... because we'd want to know
		// that a text is missing! But, we only want to report each missing text once, otherwise
		// we'll spam the console with the same warning about the same missing text over and over!
		// We used to avoid that by logging the warning, then inserting the text's name into the
		// table as its value, so that future getText() calls would find it.
		// However, now there are two places we look: the texts table, and defaultTexts. Falling
		// back to a default text should also provide some kind of lesser warning, as it could be
		// intentional. In any case, we can't only warn once unless we copy the default into texts,
		// but then when the defaults file is unloaded, it won't unload the copy (though it will
		// erase the memory that the copy points to) so we can't do that.
		//
		// Had two different ideas for solutions. One is we don't store Strings, but a wrapper that
		// also has a "thing was missing" flag somehow. That doesn't really work when I think about
		// it though... Idea 2 is to keep a Set of missing strings, add stuff to it here, and write
		// it to a file at some point. (eg, any time the set gets more members.)


		// What we're doing for now is to only report a missing text if it's not in the missingTextIDs
		// set. (And then add it.)

		if (add(&assets->missingTextIDs, name))
		{
			if (defaultText != null)
			{
				logWarn("Locale {0} is missing text for '{1}'. (Fell back to using the default locale.)"_s, {getLocale(), name});
			}
			else
			{
				logWarn("Locale {0} is missing text for '{1}'. (No default found!)"_s, {getLocale(), name});
			}
		}
	}

	return result;
}

String getAssetPath(AssetType type, String shortName)
{
	String result = shortName;

	switch (type)
	{
	case AssetType_Cursor:
		result = myprintf("{0}/cursors/{1}"_s, {assets->assetsPath, shortName}, true);
		break;
	case AssetType_BitmapFont:
		result = myprintf("{0}/fonts/{1}"_s,    {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Shader:
		result = myprintf("{0}/shaders/{1}"_s,  {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texts:
		result = myprintf("{0}/locale/{1}"_s, {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texture:
		result = myprintf("{0}/textures/{1}"_s, {assets->assetsPath, shortName}, true);
		break;
	default:
		result = myprintf("{0}/{1}"_s, {assets->assetsPath, shortName}, true);
		break;
	}

	return result;
}

void reloadLocaleSpecificAssets()
{
	// Clear the list of missing texts because they might not be missing in the new locale!
	clear(&assets->missingTextIDs);

	for (auto it = iterate(&assets->allAssets); hasNext(&it); next(&it))
	{
		Asset *asset = get(&it);
		if (asset->flags & Asset_IsLocaleSpecific)
		{
			unloadAsset(asset);
		}
	}

	for (auto it = iterate(&assets->allAssets); hasNext(&it); next(&it))
	{
		Asset *asset = get(&it);
		if (asset->flags & Asset_IsLocaleSpecific)
		{
			loadAsset(asset);
		}
	}

	assets->assetReloadHasJustHappened = true;
}

void loadCursorDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	// We store the cursorNames array in the defs asset
	// So, we first need to scan through the file to see how many cursors there are in it!
	s32 cursorCount = 0;
	while (loadNextLine(&reader))
	{
		cursorCount++;
	}

	asset->data = assetsAllocate(assets, sizeof(String) * cursorCount);
	asset->cursorDefs.cursorNames = makeArray(cursorCount, (String *) asset->data.memory);
	s32 cursorIndex = 0;

	restart(&reader);

	while (loadNextLine(&reader))
	{
		String name     = intern(&assets->assetStrings, readToken(&reader));
		String filename = readToken(&reader);
		asset->cursorDefs.cursorNames[cursorIndex++] = name;

		Maybe<s32> hotX = readInt<s32>(&reader);
		Maybe<s32> hotY = readInt<s32>(&reader);

		if (hotX.isValid && hotY.isValid)
		{
			// Add the cursor
			Asset *cursorAsset = addAsset(AssetType_Cursor, name, 0);
			cursorAsset->cursor.imageFilePath = intern(&assets->assetStrings, getAssetPath(AssetType_Cursor, filename));
			cursorAsset->cursor.hotspot = v2i(hotX.value, hotY.value);
		}
		else
		{
			error(&reader, "Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'."_s);
			return;
		}
	}
}

void loadPaletteDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	// We store the paletteNames array in the defs asset
	// So, we first need to scan through the file to see how many palettes there are in it!
	s32 paletteCount = 0;
	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);
		if (equals(command, ":Palette"_s))
		{
			paletteCount++;
		}
	}

	asset->data = assetsAllocate(assets, sizeof(String) * paletteCount);
	asset->paletteDefs.paletteNames = makeArray(paletteCount, (String *) asset->data.memory);
	s32 paletteIndex = 0;

	restart(&reader);

	Asset *paletteAsset = null;
	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);
		if (command[0] == ':')
		{
			command.length--;
			command.chars++;

			if (equals(command, "Palette"_s))
			{
				String paletteName = intern(&assets->assetStrings, readToken(&reader));
				paletteAsset = addAsset(AssetType_Palette, paletteName, 0);
				asset->paletteDefs.paletteNames[paletteIndex++] = paletteName;
			}
			else
			{
				error(&reader, "Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!"_s, {command});
				return;
			}
		}
		else
		{
			if (paletteAsset == null)
			{
				error(&reader, "Unexpected command '{0}' before the start of a :Palette"_s, {command});
				return;
			}

			if (equals(command, "type"_s))
			{
				String type = readToken(&reader);

				if (equals(type, "fixed"_s))
				{
					paletteAsset->palette.type = PaletteType_Fixed;
				}
				else if (equals(type, "gradient"_s))
				{
					paletteAsset->palette.type = PaletteType_Gradient;
				}
				else
				{
					error(&reader, "Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, {type});
				}
			}
			else if (equals(command, "size"_s))
			{
				Maybe<s32> size = readInt<s32>(&reader);
				if (size.isValid)
				{
					paletteAsset->palette.size = size.value;
				}
			}
			else if (equals(command, "color"_s))
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
							error(&reader, "Too many 'color' definitions! 'size' must be large enough."_s);
						}
						else
						{
							paletteAsset->palette.paletteData[colorIndex] = color.value;
						}
					}
					else
					{
						error(&reader, "'color' is only a valid command for fixed palettes."_s);
					}
				}
			}
			else if (equals(command, "from"_s))
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
						error(&reader, "'from' is only a valid command for gradient palettes."_s);
					}
				}
			}
			else if (equals(command, "to"_s))
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
						error(&reader, "'to' is only a valid command for gradient palettes."_s);
					}
				}
			}
			else
			{
				error(&reader, "Unrecognised command '{0}'"_s, {command});
			}
		}
	}
}

void loadSpriteDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	Asset *textureAsset = null;
	V2I spriteSize   = v2i(0,0);
	V2I spriteBorder = v2i(0,0);
	Asset *spriteGroup = null;
	s32 spriteIndex = 0;

	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);

		if (command.chars[0] == ':') // Definitions
		{
			// Define something
			command.chars++;
			command.length--;

			if (equals(command, "SpriteGroup"_s))
			{
				String name              = readToken(&reader);
				String filename          = readToken(&reader);
				Maybe<V2I> spriteSizeIn  = readV2I  (&reader);

				if (isEmpty(name) || isEmpty(filename) || !spriteSizeIn.isValid)
				{
					error(&reader, "Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
					return;
				}

				textureAsset = addTexture(filename, false);
				spriteSize = spriteSizeIn.value;

				s32 spriteCount = countPropertyOccurrences(&reader, "sprite"_s);
				if (spriteCount < 1)
				{
					error(&reader, "SpriteGroup must contain at least 1 sprite!"_s);
					return;
				}
				spriteGroup = addSpriteGroup(name, spriteCount);
				spriteIndex = 0;
			}
			else
			{
				error(&reader, "Unrecognised command: '{0}'"_s, {command});
				return;
			}
		}
		else // Properties!
		{
			if (spriteGroup == null)
			{
				error(&reader, "Found a property before starting a :SpriteGroup!"_s);
				return;
			}
			else if (equals(command, "border"_s))
			{
				Maybe<s32> borderW = readInt<s32>(&reader);
				Maybe<s32> borderH = readInt<s32>(&reader);
				if (borderW.isValid && borderH.isValid)
				{
					spriteBorder = v2i(borderW.value, borderH.value);
				}
				else
				{
					error(&reader, "Couldn't parse border. Expected 'border width height'."_s);
					return;
				}
			}
			else if (equals(command, "sprite"_s))
			{
				Maybe<s32> mx = readInt<s32>(&reader);
				Maybe<s32> my = readInt<s32>(&reader);

				if (mx.isValid && my.isValid)
				{
					s32 x = mx.value;
					s32 y = my.value;

					Sprite *sprite = spriteGroup->spriteGroup.sprites + spriteIndex;
					sprite->texture = textureAsset;
					sprite->uv = rectXYWHi(spriteBorder.x + x * (spriteSize.x + spriteBorder.x + spriteBorder.x),
										   spriteBorder.y + y * (spriteSize.y + spriteBorder.y + spriteBorder.y),
										   spriteSize.x, spriteSize.y);

					spriteIndex++;
				}
				else
				{
					error(&reader, "Couldn't parse {0}. Expected '{0} x y'."_s, {command});
					return;
				}
			}
			else
			{
				error(&reader, "Unrecognised command '{0}'"_s, {command});
				return;
			}
		}
	}
}

void removeSpriteDefs(Array<String> namesToRemove)
{
	for (s32 nameIndex = 0; nameIndex < namesToRemove.count; nameIndex++)
	{
		String spriteGroupName = namesToRemove[nameIndex];
		removeAsset(AssetType_Sprite, spriteGroupName);
	}
}

void loadTexts(HashTable<String> *texts, Asset *asset, Blob fileData)
{
	LineReader reader = readLines(asset->shortName, fileData);

	// NB: We store the strings inside the asset data, so it's one block of memory instead of many small ones.
	// However, since we allocate before we parse the file, we need to make sure that the output texts are
	// never longer than the input texts, or we could run out of space!
	// Right now, the only processing we do is replacing \n with a newline character, and similar, so the
	// output can only ever be smaller or the same size as the input.
	// - Sam, 01/10/2019

	// We also put an array of keys into the same allocation.
	// We use the number of lines in the file as a heuristic - we know it'll be slightly more than
	// the number of texts in the file, because they're 1 per line, and we don't have many blanks.

	s32 lineCount = countLines(fileData);
	smm keyArraySize = sizeof(String) * lineCount;
	asset->data = assetsAllocate(assets, fileData.size + keyArraySize);

	asset->texts.keys = makeArray(lineCount, (String*)asset->data.memory);
	s32 keyCount = 0;

	smm currentSize = keyArraySize;
	char *currentPos = (char *)(asset->data.memory + keyArraySize);

	while (loadNextLine(&reader))
	{
		String inputKey = readToken(&reader);
		String inputText = getRemainderOfLine(&reader);

		// Store the key
		ASSERT(currentSize + inputKey.length <= asset->data.size);
		String key = makeString(currentPos, inputKey.length, false);
		copyString(inputKey, &key);
		currentSize += key.length;
		currentPos += key.length;

		// Store the text
		ASSERT(currentSize + inputText.length <= asset->data.size);
		String text = makeString(currentPos, 0, false);

		for (s32 charIndex = 0; charIndex < inputText.length; charIndex++)
		{
			char c = inputText[charIndex];
			if (c == '\\')
			{
				if (((charIndex + 1) < inputText.length)
					&& (inputText[charIndex + 1] == 'n'))
				{
					text.chars[text.length] = '\n';
					text.length++;
					charIndex++;
					continue;
				}
			}

			text.chars[text.length] = c;
			text.length++;
		}

		currentSize += text.length;
		currentPos += text.length;

		// Check that we don't already have a text with that name.
		// If we do, one will overwrite the other, and that could be unpredictable if they're
		// in different files. Things will still work, but it would be confusing! And unintended.
		String *existingValue = find(texts, key);
		if (existingValue != null && !equals(*existingValue, key))
		{
			warn(&reader, "Text asset with ID '{0}' already exists in the texts table! Existing value: \"{1}\""_s, {key, *existingValue});
		}

		asset->texts.keys[keyCount++] = key;

		put(texts, key, text);
	}

	asset->texts.keys.count = keyCount;
}
