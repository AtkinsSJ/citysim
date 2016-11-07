#pragma once

enum TextureAtlasItem
{
	TextureAtlasItem_None,

	TextureAtlasItem_Map1,

	TextureAtlasItemCount
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

struct AssetManager
{
	MemoryArena arena;

	uint32 textureCount;
	Texture textures[32];
};

#include "assets.cpp"