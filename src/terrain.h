#pragma once

enum TerrainType
{
	Terrain_Invalid = 0,
	Terrain_Ground = 1,
	Terrain_Water = 2,
	Terrain_Forest = 3,

	Terrain_Size
};

struct TerrainDef
{
	TerrainType type;
	u32 textureAssetType;

	bool canBuildOn;
	bool canDemolish;
	s32 demolishCost;
};

Array<TerrainDef> terrainDefs = {};

struct Terrain
{
	TerrainType type;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
};

Terrain invalidTerrain = {Terrain_Invalid, 0};


void loadTerrainDefinitions(Array<TerrainDef> *terrains, AssetManager *assets, File file);