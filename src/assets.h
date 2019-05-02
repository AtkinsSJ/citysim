#pragma once

enum AssetType
{
	AssetType_Misc,

	AssetType_BitmapFont,
	AssetType_BuildingDefs,
	AssetType_Cursor,
	AssetType_Keymap,
	AssetType_Shader,
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

struct Shader
{
	String shaderHeaderFilename;
	String vertexShaderFilename;
	String fragmentShaderFilename;

	String vertexShader;
	String fragmentShader;
};

struct Asset
{
	AssetType type;

	// shortName = "foo.txt", fullName = "c:/whatever/foo.txt"
	String shortName;
	String fullName;

	AssetState state;

	// Depending on the AssetType, this could be the file contents, or something else!
	smm size;
	u8* memory;

	// Type-specific stuff
	// TODO: This union represents type-specific data size, which affects ALL assets!
	// We might want to move everything into the *memory pointer, so that each Asset is only as big
	// as it needs to be. The Shader struct is 65+ bytes (not including padding) which really inflates
	// the other asset types. Though 65 isn't much, so maybe it's not a big deal. We'll see if we
	// have anything bigger later.
	union {
		struct {
			SDL_Cursor *sdlCursor;
		} cursor;

		Shader shader;

		struct {
			bool isFileAlphaPremultiplied;
			SDL_Surface *surface;
		} texture;

		BitmapFont bitmapFont;
	};
};

enum ShaderType
{
	Shader_Textured,
	Shader_Untextured,
	Shader_PixelArt,

	ShaderCount,
	Shader_Invalid = -1
};

struct Texture_Temp
{
	String filename;
	s32 assetIndex;
};

struct BitmapFont_Meta
{
	String name;
	s32 assetIndex;
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
	ChunkedArray<Texture_Temp> textureIndexToAssetIndex;

	// NB: index 0 is reserved as a null sprite.
	ChunkedArray<Sprite> sprites;

	// NOTE: At each index is the first or last position in sprites array matching that type.
	// So, assets with the same type must be contiguous!
	ChunkedArray<IndexRange> rangesBySpriteAssetType;

	ChunkedArray<s32> cursorTypeToAssetIndex;
	ChunkedArray<s32> shaderTypeToAssetIndex;

	ChunkedArray<BitmapFont_Meta> fonts;

	// TODO: This stuff is in the assetArena, maybe we should migrate it to each of the styles being an Asset?
	UITheme theme;
	ChunkedArray<UIButtonStyle>  buttonStyles;
	ChunkedArray<UILabelStyle>   labelStyles;
	ChunkedArray<UITooltipStyle> tooltipStyles;
	ChunkedArray<UIMessageStyle> messageStyles;
	ChunkedArray<UITextBoxStyle> textBoxStyles;
	ChunkedArray<UIWindowStyle>  windowStyles;

	
	// TODO: We eventually want some kind of by-name lookup rather than hard-coded indices!
	s32 creditsAssetIndex;

	/*

	TODO
	----

	Our current Asset struct, which keeps a copy of the file contents in memory, is not actually what we want!
	A bunch of assets actually want the file to be loaded, then processed, and the result should be what's
	kept in the Asset, because we never look at the source file's data ever again!
	eg, for Fonts, we probably want the memory* pointer to be to the glyph data so it's easy to locate.

	Asset types:
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

Asset *getTexture(AssetManager *assets, u32 textureIndex)
{
	return getAsset(assets, get(&assets->textureIndexToAssetIndex, textureIndex)->assetIndex);
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

BitmapFont *getFont(AssetManager *assets, s32 assetIndex)
{
	return &getAsset(assets, assetIndex)->bitmapFont;
}

BitmapFont *findFont(AssetManager *assets, String fontName)
{
	BitmapFont *result = null;

	for (auto it = iterate(&assets->fonts); !it.isDone; next(&it))
	{
		auto font = get(it);
		if (equals(font->name, fontName))
		{
			result = getFont(assets, font->assetIndex);
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

Shader *getShader(AssetManager *assets, ShaderType shaderID)
{
	return &getAsset(assets, *get(&assets->shaderTypeToAssetIndex, shaderID))->shader;
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
	case AssetType_BitmapFont:
		result = myprintf("{0}/fonts/{1}",    {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Shader:
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
inline String getAssetPath(AssetManager *assets, AssetType type, char *shortName)
{
	return getAssetPath(assets, type, makeString(shortName));
}

String getUserDataPath(AssetManager *assets, String shortName)
{
	return myprintf("{0}{1}", {assets->userDataPath, shortName}, true);
}

String getUserSettingsPath(AssetManager *assets)
{
	return getUserDataPath(assets, makeString("settings.cnf"));
}