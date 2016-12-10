#pragma once

AssetManager *createAssetManager()
{
	AssetManager *assets;
	bootstrapArena(AssetManager, assets, arena, MB(128));

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
	assets->theme.buttonStyle.textColor        = color255(   0,   0,   0, 255 );
	assets->theme.buttonStyle.backgroundColor  = color255( 255, 255, 255, 255 );
	assets->theme.buttonStyle.hoverColor 	     = color255( 192, 192, 255, 255 );
	assets->theme.buttonStyle.pressedColor     = color255( 128, 128, 255, 255 );

	assets->theme.labelColor             = color255( 255, 255, 255, 255 );
	assets->theme.overlayColor           = color255(   0,   0,   0, 128 );

	assets->theme.textboxBackgroundColor = color255( 255, 255, 255, 255 );
	assets->theme.textboxTextColor       = color255(   0,   0,   0, 255 );
	
	assets->theme.tooltipBackgroundColor = color255(   0,   0,   0, 128 );
	assets->theme.tooltipColorNormal     = color255( 255, 255, 255, 255 );
	assets->theme.tooltipColorBad        = color255( 255,   0,   0, 255 );

	assets->theme.font       = FontAssetType_Main;
	assets->theme.buttonStyle.font = FontAssetType_Buttons;
}

int32 addTexture(AssetManager *assets, char *filename, bool isAlphaPremultiplied)
{
	ASSERT(assets->textureCount < ArrayCount(assets->textures), "No room for texture");

	int32 textureID = assets->textureCount++;

	Texture *texture = assets->textures + textureID;
	texture->filename = pushString(&assets->arena, filename);
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

void addCursor(AssetManager *assets, CursorType cursorID, char *filename)
{
	Cursor *cursor = assets->cursors + cursorID;
	cursor->filename = filename;
	cursor->sdlCursor = 0;
}

BitmapFont *addBMFont(AssetManager *assets, TemporaryMemoryArena *tempArena, FontAssetType fontAssetType,
	                  TextureAssetType textureAssetType, char *filename);

void addShaderProgram(AssetManager *assets, ShaderProgramType shaderID, char *vertFilename, char *fragFilename)
{
	ShaderProgram *shader = assets->shaderPrograms + shaderID;
	shader->fragFilename = fragFilename;
	shader->vertFilename = vertFilename;
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
	for (uint32 cursorID = 1; cursorID < CursorCount; cursorID++)
	{
		Cursor *cursor = assets->cursors + cursorID;

		SDL_Surface *cursorSurface = IMG_Load(cursor->filename);
		cursor->sdlCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		SDL_FreeSurface(cursorSurface);
	}

	// Load shader programs
	for (uint32 shaderID = 0; shaderID < ShaderProgramCount; shaderID++)
	{
		ShaderProgram *shader = assets->shaderPrograms + shaderID;
		shader->vertShader = readFileAsString(&assets->arena, shader->vertFilename);
		shader->fragShader = readFileAsString(&assets->arena, shader->fragFilename);

		if (shader->vertShader && shader->fragShader)
		{
			shader->state = AssetState_Loaded;
		}
		else
		{
			ASSERT(false, "Failed to load shader program %d.", shaderID);
		}
	}

	initTheme(assets);
}

void addAssets(AssetManager *assets, MemoryArena *memoryArena)
{
	addTextureRegion(assets, TextureAssetType_Map1, "London-Strand-Holbron-Bloomsbury.png",
	                 rectXYWH(0,0,2002,1519), false);
	TemporaryMemoryArena tempMemory = beginTemporaryMemory(memoryArena);
	addBMFont(assets, &tempMemory, FontAssetType_Buttons, TextureAssetType_Font_Buttons, "dejavu-14.fnt");
	addBMFont(assets, &tempMemory, FontAssetType_Main, TextureAssetType_Font_Main, "dejavu-20.fnt");
	endTemporaryMemory(&tempMemory);

	addShaderProgram(assets, ShaderProgram_Textured, "textured.vert.gl", "textured.frag.gl");
	addShaderProgram(assets, ShaderProgram_Untextured, "untextured.vert.gl", "untextured.frag.gl");

	addCursor(assets, Cursor_Main, "cursor_main.png");
	addCursor(assets, Cursor_Build, "cursor_build.png");
	addCursor(assets, Cursor_Demolish, "cursor_demolish.png");
	addCursor(assets, Cursor_Plant, "cursor_plant.png");
	addCursor(assets, Cursor_Harvest, "cursor_harvest.png");
	addCursor(assets, Cursor_Hire, "cursor_hire.png");
}

void reloadAssets(AssetManager *assets, MemoryArena *memoryArena)
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
	assets->textureCount = 1;
	assets->textureRegionCount = 1;
	resetMemoryArena(&assets->arena);
	addAssets(assets, memoryArena);
	loadAssets(assets);

}