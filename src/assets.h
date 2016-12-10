#pragma once

enum TextureAssetType
{
	TextureAssetType_None,

	TextureAssetType_Font_Main,
	TextureAssetType_Font_Buttons,

	TextureAssetType_Map1,

	TextureAssetTypeCount
};

enum FontAssetType
{
	FontAssetType_Main,
	FontAssetType_Buttons,

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
	char *filename;
	bool isAlphaPremultiplied; // Is the source file premultiplied?
	SDL_Surface *surface;
};

struct TextureRegion
{
	TextureAssetType type;
	int32 textureID;
	RealRect uv; // in (0 to 1) space
};

#include "font.h"

struct Cursor
{
	char *filename;
	SDL_Cursor *sdlCursor;
};

struct ShaderProgram
{
	AssetState state;
	char *fragFilename;
	char *vertFilename;

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

struct UITheme
{
	FontAssetType font;

	V4 overlayColor;
			
	V4 labelColor;

	V4 textboxTextColor,
		textboxBackgroundColor;

	V4 tooltipBackgroundColor,
		tooltipColorNormal,
		tooltipColorBad;

	UIButtonStyle buttonStyle;
};

struct AssetManager
{
	MemoryArena arena;

	// NB: index 0 reserved as a null texture.
	uint32 textureCount;
	Texture textures[32];

	// NB: index 0 is reserved as a null region.
	uint32 textureRegionCount;
	TextureRegion textureRegions[8192];

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	int32 firstIDForTextureAssetType[TextureAssetTypeCount];
	int32 lastIDForTextureAssetType[TextureAssetTypeCount];

	ShaderProgram shaderPrograms[ShaderProgramCount];

	BitmapFont fonts[FontAssetTypeCount];

	Cursor cursors[CursorCount];
	CursorType activeCursor;

	UITheme theme;
};

int32 getTextureRegion(AssetManager *assets, TextureAssetType item, int32 offset)
{
	int32 min = assets->firstIDForTextureAssetType[item],
		  max = assets->lastIDForTextureAssetType[item];

	int32 id = clampToRangeWrapping(min, max, offset);
	ASSERT((id >= min) && (id <= max), "Got a textureRegionId outside of the range.");

	return id;
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

#include "assets.cpp"