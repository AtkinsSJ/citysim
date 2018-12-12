#pragma once

enum AssetType
{
	AssetType_Misc,
	AssetType_Cursor,
	AssetType_Font,
	AssetType_Shader,
	AssetType_Texture,
};

enum TextureAssetType // NB: Actually TextureRegions, unless I change that later.
{
	TextureAssetType_None = 0,

	TextureAssetType_Font_Main = 1,
	TextureAssetType_Font_Buttons = 2,
	TextureAssetType_Font_Debug = 3,

	TextureAssetType_Map1 = 4,

	TextureAssetType_GroundTile = 5,
	TextureAssetType_ForestTile = 6,
	TextureAssetType_WaterTile = 7,

	TextureAssetType_Road = 8,
	TextureAssetType_House_2x2 = 9,
	TextureAssetType_Factory_3x3 = 10,
	TextureAssetType_PowerStation = 11,
	TextureAssetType_PowerLine = 12,

	TextureAssetTypeCount
};

enum FontAssetType
{
	FontAssetType_Main,
	FontAssetType_Buttons,
	FontAssetType_Debug,

	FontAssetTypeCount
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

enum ShaderProgramType
{
	ShaderProgram_Textured,
	ShaderProgram_Untextured,

	ShaderProgramCount,
	ShaderProgram_Invalid = -1
};

enum AssetState
{
	AssetState_Unloaded,
	AssetState_Loaded,
};

struct Texture
{
	AssetState state;
	String filename;
	bool isAlphaPremultiplied; // Is the source file premultiplied?
	SDL_Surface *surface;
};

struct TextureRegion
{
	TextureAssetType type;
	s32 textureID;
	Rect2 uv; // in (0 to 1) space
};

// struct TextureRegionList
// {
// 	DLinkedListMembers(TextureRegionList);
// 	u32 usedCount;
// 	TextureRegion regions[512]; // TODO: Tune this for performance!
// };

typedef u32 TextureRegionID;

#include "font.h"

struct Cursor
{
	String filename;
	SDL_Cursor *sdlCursor;
};

struct ShaderProgram
{
	AssetState state;
	String fragFilename;
	String vertFilename;

	String fragShader;
	String vertShader;
};

struct ShaderHeader
{
	AssetState state;
	String filename;
	String contents;
};

#include "uitheme.cpp"

struct IndexRange
{
	u32 firstIndex;
	u32 lastIndex;
};

struct AssetManager
{
	MemoryArena assetArena;

	String assetsPath;

	// NB: index 0 reserved as a null texture.
	ChunkedArray<Texture> textures;

	// NB: index 0 is reserved as a null region.
	ChunkedArray<TextureRegion> textureRegions;

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	ChunkedArray<IndexRange> rangesByTextureAssetType;

	ShaderHeader shaderHeader; // This is a bit hacky right now.
	ShaderProgram shaderPrograms[ShaderProgramCount];

	BitmapFont fonts[FontAssetTypeCount];

	Cursor cursors[CursorCount];
	CursorType activeCursor;

	UITheme theme;
	File creditsText;
};

Texture *getTexture(AssetManager *assets, u32 textureIndex)
{
	return get(&assets->textures, textureIndex);
}

TextureRegionID getTextureRegionID(AssetManager *assets, TextureAssetType item, u32 offset)
{
	IndexRange *range = get(&assets->rangesByTextureAssetType, item);
	u32 min = range->firstIndex;
	u32 max = range->lastIndex;

	u32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a textureRegionId outside of the range.");

	return id;
}

TextureRegion *getTextureRegion(AssetManager *assets, TextureRegionID textureRegionIndex)
{
	return get(&assets->textureRegions, textureRegionIndex);
}

BitmapFont *getFont(AssetManager *assets, FontAssetType font)
{
	return assets->fonts + font;
}

Cursor *getCursor(AssetManager *assets, CursorType cursorID)
{
	ASSERT((cursorID > Cursor_None) && (cursorID < CursorCount), "Cursor ID out of range: %d", cursorID);
	return assets->cursors + cursorID;
}

ShaderProgram *getShaderProgram(AssetManager *assets, ShaderProgramType shaderID)
{
	ASSERT((shaderID > -1) && (shaderID < ShaderProgramCount), "Shader ID out of range: %d", shaderID);
	return assets->shaderPrograms + shaderID;
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
		result = myprintf("{0}/fonts/{1}", {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Shader:
		result = myprintf("{0}/shaders/{1}", {assets->assetsPath, shortName}, true);
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