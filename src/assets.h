#pragma once

enum AssetType
{
	AssetType_Misc,

	AssetType_BuildingDefs,

	AssetType_Cursor,
	AssetType_Font,
	
	AssetType_ShaderProgram,
	AssetType_ShaderPart,

	AssetType_Texture,

	AssetType_TerrainDefs,

	AssetType_UITheme,
};

enum AssetState
{
	AssetState_Unloaded,
	AssetState_Loaded,
};

enum CursorType
{
	Cursor_None,

	Cursor_Main,
	Cursor_Build,
	Cursor_Demolish,
	Cursor_Plant,
	Cursor_Harvest,
	Cursor_Hire,

	CursorCount
};

struct Asset
{
	AssetType type;

	// shortName = "foo.txt", fullName = "c:/whatever/foo.txt"
	String shortName;
	String fullName;

	AssetState state;
	smm size;
	u8* memory;

	// Type-specific stuff
	union {
		struct {
			SDL_Cursor *sdlCursor;
		} cursor;

		struct {
			s32 fragShaderAssetIndex;
			s32 vertShaderAssetIndex;
		} shaderProgram;
	};
};

enum ShaderProgramType
{
	ShaderProgram_Textured,
	ShaderProgram_Untextured,
	ShaderProgram_PixelArt,

	ShaderProgramCount,
	ShaderProgram_Invalid = -1
};

struct Texture
{
	AssetState state;
	String filename;
	bool isAlphaPremultiplied; // Is the source file premultiplied?
	SDL_Surface *surface;
};

struct Sprite
{
	// Identifier so multiple sprites can be associated with the same id.
	// eg, for getting a random sprite from a list, or we also use this for glyphs within a font.
	s32 spriteAssetType;

	s32 textureID;
	Rect2 uv; // in (0 to 1) space
};

struct IndexRange
{
	u32 firstIndex;
	u32 lastIndex;
};

struct AssetManager
{
	MemoryArena assetArena;
	bool assetReloadHasJustHappened;
	smm assetMemoryAllocated;
	smm maxAssetMemoryAllocated;

	String assetsPath;
	String userDataPath;

	ChunkedArray<Asset> allAssets;

	// NB: index 0 reserved as a null texture.
	ChunkedArray<Texture> textures;

	// NB: index 0 is reserved as a null sprite.
	ChunkedArray<Sprite> sprites;

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	ChunkedArray<IndexRange> rangesBySpriteAssetType;

	ChunkedArray<s32> cursorTypeToAssetIndex;
	ChunkedArray<s32> shaderProgramTypeToAssetIndex;

	ChunkedArray<BitmapFont> fonts;
	ChunkedArray<UIButtonStyle>  buttonStyles;
	ChunkedArray<UILabelStyle>   labelStyles;
	ChunkedArray<UITooltipStyle> tooltipStyles;
	ChunkedArray<UIMessageStyle> messageStyles;
	ChunkedArray<UITextBoxStyle> textBoxStyles;
	ChunkedArray<UIWindowStyle>  windowStyles;

	UITheme theme;
	
	// TODO: We eventually want some kind of by-name lookup rather than hard-coded indices!
	s32 creditsAssetIndex;

	s32 shaderHeaderAssetIndex;

	/*

	TODO
	----

	Asset types:
	- font assets
	- texture assets
	- sprites somehow??? There are thousands of them, because of fonts... maybe font glyphs should use a different, more specialised system?

	Much later...
	- audio! (sfx/music as separate things probably)
	- localised text

	Other changes:
	- lookup by name
	- reload individual files
	- automated reloading
	- automated cataloguing

	*/
};

Asset *getAsset(AssetManager *assets, s32 assetIndex)
{
	Asset *asset = get(&assets->allAssets, assetIndex);
	// TODO: load it if it's not loaded?
	return asset;
}

Texture *getTexture(AssetManager *assets, u32 textureIndex)
{
	return get(&assets->textures, textureIndex);
}

s32 getSpriteID(AssetManager *assets, s32 spriteAssetType, u32 offset)
{
	IndexRange *range = get(&assets->rangesBySpriteAssetType, spriteAssetType);
	u32 min = range->firstIndex;
	u32 max = range->lastIndex;

	u32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a sprite id outside of the range.");

	return id;
}

Sprite *getSprite(AssetManager *assets, s32 spriteIndex)
{
	return get(&assets->sprites, spriteIndex);
}

BitmapFont *getFont(AssetManager *assets, s32 fontID)
{
	return get(&assets->fonts, fontID);
}

BitmapFont *getFont(AssetManager *assets, String fontName)
{
	BitmapFont *result = null;

	for (auto it = iterate(&assets->fonts); !it.isDone; next(&it))
	{
		auto font = get(it);
		if (equals(font->name, fontName))
		{
			result = font;
			break;
		}
	}

	if (result == null)
	{
		logError("Requested font '{0}' was not found!", {fontName});
	}

	return result;
}

// TODO: Remove this!
Asset *getCursor(AssetManager *assets, u32 cursorID)
{
	return getAsset(assets, *get(&assets->cursorTypeToAssetIndex, cursorID));
}

Asset *getShaderProgram(AssetManager *assets, ShaderProgramType shaderID)
{
	return getAsset(assets, *get(&assets->shaderProgramTypeToAssetIndex, shaderID));
}

// TODO: A way to get this as a String might be more convenient!
Asset *getTextAsset(AssetManager *assets, s32 assetIndex)
{
	Asset *asset = get(&assets->allAssets, assetIndex);
	// TODO: load it if it's not loaded?
	return asset;
}

String getAssetPath(AssetManager *assets, AssetType type, String shortName)
{
	String result = shortName;

	switch (type)
	{
	case AssetType_Cursor:
		result = myprintf("{0}/textures/{1}", {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Font:
		result = myprintf("{0}/fonts/{1}",    {assets->assetsPath, shortName}, true);
		break;
	case AssetType_ShaderProgram:
		result = nullString;
		break;
	case AssetType_ShaderPart:
		result = myprintf("{0}/shaders/{1}",  {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texture:
		result = myprintf("{0}/textures/{1}", {assets->assetsPath, shortName}, true);
		break;
	default:
		result = myprintf("{0}/{1}",          {assets->assetsPath, shortName}, true);
		break;
	}

	return result;
}

String getUserDataPath(AssetManager *assets, String shortName)
{
	return myprintf("{0}{1}", {assets->userDataPath, shortName}, true);
}

String getUserSettingsPath(AssetManager *assets)
{
	return getUserDataPath(assets, makeString("settings.cnf"));
}

BitmapFont *addBMFont(AssetManager *assets, String name, String filename);