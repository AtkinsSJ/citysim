#pragma once

struct TerrainDef
{
	String name;

	String spriteName;
	SpriteGroup *sprites;

	bool canBuildOn;
};

ChunkedArray<TerrainDef> terrainDefs = {};

struct Terrain
{
	s32 type;
	s32 spriteOffset; // used as the offset for getSprite
};

Terrain invalidTerrain = {0, 0};


void loadTerrainDefinitions(ChunkedArray<TerrainDef> *terrains, AssetManager *assets, Blob data, Asset *asset);
void refreshTerrainSpriteCache(ChunkedArray<TerrainDef> *terrains, AssetManager *assets);

// Returns 0 if not found
s32 findTerrainTypeByName(String name);
