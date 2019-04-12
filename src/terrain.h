#pragma once

struct TerrainDef
{
	String name;

	u32 textureAssetType;

	bool canBuildOn;
	bool canDemolish;
	s32 demolishCost;
};

ChunkedArray<TerrainDef> terrainDefs = {};

struct Terrain
{
	u32 type;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
};

Terrain invalidTerrain = {0, 0};


void loadTerrainDefinitions(ChunkedArray<TerrainDef> *terrains, AssetManager *assets, Asset *asset);

// Returns 0 if not found
s32 findTerrainTypeByName(String name)
{
	s32 result = 0;

	for (s32 terrainID = 1; terrainID < terrainDefs.count; terrainID++)
	{
		TerrainDef *def = get(&terrainDefs, terrainID);
		if (equals(def->name, name))
		{
			result = terrainID;
			break;
		}
	}

	return result;
}