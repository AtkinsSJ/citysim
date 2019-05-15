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

void loadAsset(AssetManager *assets, Asset *asset)
{
	ASSERT(asset->state == AssetState_Unloaded, "Attempted to load an asset ({0}) that is already loaded!", {asset->fullName});
	DEBUG_FUNCTION();

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
			Blob headerFile   = readTempFile(asset->shader.shaderHeaderFilename);
			Blob vertexFile   = readTempFile(asset->shader.vertexShaderFilename);
			Blob fragmentFile = readTempFile(asset->shader.fragmentShaderFilename);
			
			// nb: extra 1 byte each, for newlines between the header and main source
			smm vertexSize   = headerFile.size + vertexFile.size   + 1;
			smm fragmentSize = headerFile.size + fragmentFile.size + 1;
			smm totalSize    = fragmentSize + vertexSize;

			asset->data = allocate(assets, totalSize);

			asset->shader.vertexShader   = makeString((char*)asset->data.memory, truncate32(vertexSize));
			asset->shader.fragmentShader = makeString((char*)asset->data.memory + vertexSize, truncate32(fragmentSize));

			smm offset = 0;

			memcpy(asset->data.memory + offset, headerFile.memory, headerFile.size);
			offset += headerFile.size;
			asset->data.memory[offset++] = '\n';
			memcpy(asset->data.memory + offset, vertexFile.memory, vertexFile.size);
			offset += vertexFile.size;

			memcpy(asset->data.memory + offset, headerFile.memory, headerFile.size);
			offset += headerFile.size;
			asset->data.memory[offset++] = '\n';
			memcpy(asset->data.memory + offset, fragmentFile.memory, fragmentFile.size);
			offset += fragmentFile.size;

			ASSERT(offset == totalSize, "We copied the shader data wrong!");

			asset->state = AssetState_Loaded;
		} break;

		case AssetType_Sprite:
		{
			// Convert UVs from pixel space to 0-1 space
			for (s32 i = 0; i < asset->spriteGroup.count; i++)
			{
				Sprite *sprite = asset->spriteGroup.sprites + i;
				Asset *t = sprite->texture;
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
	Blob spritesBlob = allocate(assets, spriteCount * sizeof(Sprite));
	spriteGroup->spriteGroup.count = spriteCount;
	spriteGroup->spriteGroup.sprites = (Sprite*) spritesBlob.memory;

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

void addShader(AssetManager *assets, ShaderType shaderID, char *name, char *headerFilename, char *vertFilename, char *fragFilename)
{
	Asset *asset = addAsset(assets, AssetType_Shader, pushString(&assets->assetArena, name), false);
	asset->shader.shaderType = shaderID;
	asset->shader.shaderHeaderFilename   = pushString(&assets->assetArena, getAssetPath(assets, AssetType_Shader, headerFilename));
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

void addAssets(AssetManager *assets)
{
	addAsset(assets, AssetType_Misc,         "credits.txt");
	addAsset(assets, AssetType_UITheme,      "ui.theme");
	addAsset(assets, AssetType_BuildingDefs, "buildings.def");
	addAsset(assets, AssetType_TerrainDefs,  "terrain.def");

	addAsset(assets, AssetType_DevKeymap, "dev.keymap");

	// TODO: Settings?

	addShader(assets, Shader_Textured,   "textured",   "header.glsl", "textured.vert.glsl",   "textured.frag.glsl");
	addShader(assets, Shader_Untextured, "untextured", "header.glsl", "untextured.vert.glsl", "untextured.frag.glsl");
	addShader(assets, Shader_PixelArt,   "pixelart",   "header.glsl", "pixelart.vert.glsl",   "pixelart.frag.glsl");

	addAsset(assets, AssetType_Cursor, "default.png");
	addAsset(assets, AssetType_Cursor, "build.png");
	addAsset(assets, AssetType_Cursor, "demolish.png");
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