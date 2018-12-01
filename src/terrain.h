#pragma once

enum TerrainType
{
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Forest,
	Terrain_Size
};

struct TerrainDef
{
	TerrainType type;
	TextureAssetType textureAssetType;

	bool canBuildOn;
	bool canDemolish;
	s32 demolishCost;
};

TerrainDef terrainDefinitions[Terrain_Size] = {
	{Terrain_Invalid, TextureAssetType_None,       false, false,   0},
	{Terrain_Ground,  TextureAssetType_GroundTile, true,  false,   0},
	{Terrain_Water,   TextureAssetType_WaterTile,  false, false,   0},
	{Terrain_Forest,  TextureAssetType_ForestTile, false, true,  100},
};

struct Terrain
{
	TerrainType type;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
};

Terrain invalidTerrain = {Terrain_Invalid, 0};