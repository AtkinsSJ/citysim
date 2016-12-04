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
	Cursor_Main,
	Cursor_Build,
	Cursor_Demolish,
	Cursor_Plant,
	Cursor_Harvest,
	Cursor_Hire,

	CursorCount
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

struct UiTheme
{
	FontAssetType font,
				  buttonFont;

	V4 overlayColor;
			
	V4 labelColor;

	V4 buttonTextColor,
		buttonBackgroundColor,
		buttonHoverColor,
		buttonPressedColor;

	V4 textboxTextColor,
		textboxBackgroundColor;

	V4 tooltipBackgroundColor,
		tooltipColorNormal,
		tooltipColorBad;
};

struct AssetManager
{
	MemoryArena arena;

	uint32 textureCount;
	Texture textures[32];

	// NB: index 0 is reserved as a null region.
	uint32 textureRegionCount;
	TextureRegion textureRegions[8192];

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	int32 firstIDForTextureAssetType[TextureAssetTypeCount];
	int32 lastIDForTextureAssetType[TextureAssetTypeCount];

	BitmapFont fonts[FontAssetTypeCount];

	Cursor cursors[CursorCount];
	CursorType activeCursor;

	UiTheme theme;
};

#include "assets.cpp"