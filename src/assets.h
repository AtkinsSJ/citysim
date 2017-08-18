#pragma once

enum AssetType
{
	AssetType_Cursor,
	AssetType_Font,
	AssetType_Shader,
	AssetType_Texture,
};

enum TextureAssetType // NB: Actually TextureRegions, unless I change that later.
{
	TextureAssetType_None,

	TextureAssetType_Font_Main,
	TextureAssetType_Font_Buttons,
	TextureAssetType_Font_Debug,

	TextureAssetType_Map1,

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

struct TextureList
{
	DLinkedListMembers(TextureList);
	uint32 usedCount;
	Texture textures[32]; // TODO: Tune this for performance!
};

struct TextureRegion
{
	TextureAssetType type;
	int32 textureID;
	RealRect uv; // in (0 to 1) space
};

struct TextureRegionList
{
	DLinkedListMembers(TextureRegionList);
	uint32 usedCount;
	TextureRegion regions[512]; // TODO: Tune this for performance!
};

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

	char *fragShader;
	char *vertShader;
};

struct UIButtonStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;
};

struct UILabelStyle
{
	FontAssetType font;
	V4 textColor;
};

struct UITooltipStyle
{
	FontAssetType font;
	V4 textColorNormal;
	V4 textColorBad;

	V4 backgroundColor;
	real32 borderPadding;
	real32 depth;
};

struct UIMessageStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	real32 borderPadding;
	real32 depth;
};

struct UITheme
{
	V4 overlayColor;

	V4 textboxTextColor,
		textboxBackgroundColor;

	UIButtonStyle buttonStyle;
	UILabelStyle labelStyle;
	UITooltipStyle tooltipStyle;
	UIMessageStyle uiMessageStyle;
};

struct AssetManager
{
	MemoryArena assetArena;

	String assetsPath;

	// NB: index 0 reserved as a null texture.
	uint32 textureCount;
	TextureList firstTextureList;
	// Texture textures[32];

	// NB: index 0 is reserved as a null region.
	uint32 textureRegionCount;
	TextureRegionList firstTextureRegionList;

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	uint32 firstIDForTextureAssetType[TextureAssetTypeCount];
	uint32 lastIDForTextureAssetType[TextureAssetTypeCount];

	ShaderProgram shaderPrograms[ShaderProgramCount];

	BitmapFont fonts[FontAssetTypeCount];

	Cursor cursors[CursorCount];
	CursorType activeCursor;

	UITheme theme;
};

Texture *getTexture(AssetManager *assets, uint32 textureIndex)
{
	ASSERT(textureIndex < assets->textureCount, "Selecting unallocated Texture!");
	TextureList *list = &assets->firstTextureList;
	const uint32 texturesPerList = ArrayCount(list->textures);

	while (textureIndex >= texturesPerList)
	{
		textureIndex -= texturesPerList;
		list = list->next;
	}

	return list->textures + textureIndex;
}

uint32 getTextureRegionID(AssetManager *assets, TextureAssetType item, uint32 offset)
{
	uint32 min = assets->firstIDForTextureAssetType[item],
		  max = assets->lastIDForTextureAssetType[item];

	uint32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a textureRegionId outside of the range.");

	return id;
}

TextureRegion *getTextureRegion(AssetManager *assets, uint32 textureRegionIndex)
{
	ASSERT(textureRegionIndex < assets->textureRegionCount, "Selecting unallocated TextureRegion!");
	TextureRegionList *list = &assets->firstTextureRegionList;
	const uint32 regionsPerList = ArrayCount(list->regions);

	while (textureRegionIndex >= regionsPerList)
	{
		textureRegionIndex -= regionsPerList;
		list = list->next;
	}

	return list->regions + textureRegionIndex;
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
	INVALID_DEFAULT_CASE;
	}

	return result;
}