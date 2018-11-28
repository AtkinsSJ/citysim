#pragma once

s32 addTexture(AssetManager *assets, char *filename, bool isAlphaPremultiplied)
{
	TextureList *list = assets->firstTextureList.prev;
	if (list->usedCount >= ArrayCount(list->textures))
	{
		list = PushStruct(&assets->assetArena, TextureList);
		DLinkedListInsertBefore(list, &assets->firstTextureList);
	}

	u32 idWithinList = list->usedCount++;
	u32 textureID = assets->textureCount++;
	ASSERT(idWithinList == (textureID % ArrayCount(list->textures)), "Texture index mismatch!");

	Texture *texture = list->textures + idWithinList;

	texture->filename = pushString(&assets->assetArena, filename);
	texture->isAlphaPremultiplied = isAlphaPremultiplied;

	return textureID;
}

u32 addTextureRegion(AssetManager *assets, TextureAssetType type, s32 textureID, RealRect uv)
{
	TextureRegionList *list = assets->firstTextureRegionList.prev;
	if (list->usedCount >= ArrayCount(list->regions))
	{
		list = PushStruct(&assets->assetArena, TextureRegionList);
		DLinkedListInsertBefore(list, &assets->firstTextureRegionList);
	}

	u32 idWithinList = list->usedCount++;
	u32 textureRegionID = assets->textureRegionCount++;
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
	DLinkedListInit(&assets->firstTextureRegionList);
	DLinkedListInit(&assets->firstTextureList);

	assets->firstTextureList.usedCount = assets->textureCount = 0;
	assets->firstTextureRegionList.usedCount = assets->textureRegionCount = 0;

	addTextureRegion(assets, TextureAssetType_None, -1, {});

	Texture *nullTexture = getTexture(assets, addTexture(assets, "", true));
	nullTexture->state = AssetState_Loaded;
	nullTexture->surface = 0;

	// Have to provide defaults for these or it just breaks.
	for (u32 i = 0; i < TextureAssetTypeCount; ++i)
	{
		assets->firstIDForTextureAssetType[i] = u32Max;
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

void initTheme(UITheme *theme)
{
	theme->buttonStyle.font               = FontAssetType_Buttons;
	theme->buttonStyle.textColor          = color255(   0,   0,   0, 255 );
	theme->buttonStyle.backgroundColor    = color255( 255, 255, 255, 255 );
	theme->buttonStyle.hoverColor 	      = color255( 192, 192, 255, 255 );
	theme->buttonStyle.pressedColor       = color255( 128, 128, 255, 255 );

	theme->labelStyle.font                = FontAssetType_Main;
	theme->labelStyle.textColor           = color255( 255, 255, 255, 255 );

	theme->tooltipStyle.font              = FontAssetType_Main;
	theme->tooltipStyle.textColorNormal   = color255( 255, 255, 255, 255 );
	theme->tooltipStyle.textColorBad      = color255( 255,   0,   0, 255 );
	theme->tooltipStyle.backgroundColor   = color255(   0,   0,   0, 128 );
	theme->tooltipStyle.borderPadding     = 4;
	theme->tooltipStyle.depth             = 100;

	theme->uiMessageStyle.font            = FontAssetType_Main;
	theme->uiMessageStyle.textColor       = color255( 255, 255, 255, 255 );
	theme->uiMessageStyle.backgroundColor = color255(   0,   0,   0, 128 );
	theme->uiMessageStyle.borderPadding   = 4;
	theme->uiMessageStyle.depth           = 100;

	theme->overlayColor           = color255(   0,   0,   0, 128 );

	theme->textboxBackgroundColor = color255( 255, 255, 255, 255 );
	theme->textboxTextColor       = color255(   0,   0,   0, 255 );
	
}

s32 findTexture(AssetManager *assets, char *filename)
{
	s32 index = -1;
	for (s32 i = 0; i < (s32)assets->textureCount; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (strcmp(filename, tex->filename) == 0)
		{
			index = i;
			break;
		}
	}
	return index;
}

u32 addTextureRegion(AssetManager *assets, TextureAssetType type, char *filename, RealRect uv,
	                   bool isAlphaPremultiplied=false)
{
	s32 textureID = findTexture(assets, filename);
	if (textureID == -1)
	{
		textureID = addTexture(assets, filename, isAlphaPremultiplied);
	}

	return addTextureRegion(assets, type, textureID, uv);
}

void addCursor(AssetManager *assets, CursorType cursorID, char *filename)
{
	Cursor *cursor = assets->cursors + cursorID;
	cursor->filename = filename;
	cursor->sdlCursor = 0;
}

BitmapFont *addBMFont(AssetManager *assets, MemoryArena *tempArena, FontAssetType fontAssetType,
	                  TextureAssetType textureAssetType, char *filename);

void addShaderProgram(AssetManager *assets, ShaderProgramType shaderID, char *vertFilename,
	                  char *fragFilename)
{
	ShaderProgram *shader = assets->shaderPrograms + shaderID;
	shader->fragFilename = fragFilename;
	shader->vertFilename = vertFilename;
}

void loadAssets(AssetManager *assets)
{
	DEBUG_FUNCTION();
	for (u32 i = 1; i < assets->textureCount; ++i)
	{
		Texture *tex = getTexture(assets, i);
		if (tex->state == AssetState_Unloaded)
		{
			tex->surface = IMG_Load(tex->filename);
			ASSERT(tex->surface, "Failed to load image '%s'!\n%s", tex->filename, IMG_GetError());

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
	for (u32 regionIndex = 1; regionIndex < assets->textureRegionCount; regionIndex++)
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
		Cursor *cursor = assets->cursors + cursorID;

		SDL_Surface *cursorSurface = IMG_Load(cursor->filename);
		cursor->sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		SDL_FreeSurface(cursorSurface);
	}

	// Load shader programs
	for (u32 shaderID = 0; shaderID < ShaderProgramCount; shaderID++)
	{
		ShaderProgram *shader = assets->shaderPrograms + shaderID;
		shader->vertShader = readFileAsString(&assets->assetArena, shader->vertFilename);
		shader->fragShader = readFileAsString(&assets->assetArena, shader->fragFilename);

		if (shader->vertShader && shader->fragShader)
		{
			shader->state = AssetState_Loaded;
		}
		else
		{
			ASSERT(false, "Failed to load shader program %d.", shaderID);
		}
	}

	initTheme(&assets->theme);
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

	// Actual reloading

	// Clear out textures
	for (u32 i = 1; i < assets->textureCount; ++i)
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
	for (u32 cursorID = 0; cursorID < CursorCount; cursorID++)
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
	SDL_FreeCursor(systemWaitCursor);
	renderer->loadAssets(renderer, assets);
	consoleWriteLine("Assets reloaded successfully!", CLS_Success);
}