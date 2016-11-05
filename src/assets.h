#pragma once

enum TextureAtlasItem
{
	TextureAtlasItem_None,

	TextureAtlasItem_Map1,

	TextureAtlasItemCount
};

enum TextureState
{
	TextureState_Unloaded,
	TextureState_Loaded,
};
struct Texture
{
	TextureState state;
	char *filename;
};

struct TextureAtlas
{
	uint32 textureCount;
	Texture textures[1];
};