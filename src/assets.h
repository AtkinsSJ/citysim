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

enum AssetState
{
	AssetState_Unloaded,
	AssetState_Loaded,
};

struct Texture
{
	AssetState state;
	char *filename;
	bool isAlphaPremultiplied;
	SDL_Surface *surface;
};

struct TextureRegion
{
	TextureAssetType type;
	int32 textureID;
	RealRect uv; // in (0 to 1) space
};

#include "font.h"

struct AssetManager
{
	MemoryArena arena;

	uint32 textureCount;
	Texture textures[32];

	// NB: index 0 is reserved as a null region.
	uint32 textureRegionCount;
	TextureRegion textureRegions[256];

	// NOTE: At each index is the first or last position in textureRegions array matching that type.
	// So, assets with the same type must be contiguous!
	int32 firstIDForTextureAssetType[TextureAssetTypeCount];
	int32 lastIDForTextureAssetType[TextureAssetTypeCount];

	BitmapFont fonts[FontAssetTypeCount];
};

#include "assets.cpp"