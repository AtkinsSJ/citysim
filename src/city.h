#pragma once

enum TerrainType
{
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Forest,
	Terrain_Size
};

struct Terrain
{
	TerrainType type;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
};

Terrain invalidTerrain = {Terrain_Invalid, 0};

const int forestDemolishCost = 100;

struct BuildingDefinition
{
	union
	{
		V2I size;
		struct
		{
			s32 width;
			s32 height;
		};
	};
	char *name;
	TextureAssetType textureAtlasItem;
	s32 buildCost;
	s32 demolishCost;
	bool isPath;
};

// Farming stuff
enum BuildingArchetype
{
	BA_Road,

	BA_Count,
	BA_None = -1
};
BuildingDefinition buildingDefinitions[] = {
	// size, name, 		 image, 				 costs b/d,  isPath
	{1,1, "Road", TextureAssetType_Road, 10, 10, true}
	// {4,4, 	"Field", 	 TextureAtlasItem_Field, 200, 20,	 false},
	// {4,4, 	"Barn", 	 TextureAtlasItem_Barn,  2000, 1000, false},
	// {4,4, 	"Farmhouse", TextureAtlasItem_House, 2000, 1000, false},
	// {1,1, 	"Path", 	 TextureAtlasItem_Path,  10, 10,	 true},
};

struct Building
{
	BuildingArchetype archetype;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

struct PathLayer
{
	s32 pathGroupCount;
	s32 *data; // Represents the pathing 'group'. 0 = unpathable, >0 = any tile with the same value is connected
};

struct City
{
	String name;
	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	Terrain *terrain;
	PathLayer pathLayer;

	u32 buildingCount;
	u32 buildingCountMax;
	Building buildings[1024]; // TODO: Make the number of buildings unlimited!
	u32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null).
};

inline u32 tileIndex(City *city, s32 x, s32 y)
{
	return (y * city->width) + x;
}

inline bool tileExists(City *city, s32 x, s32 y)
{
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Terrain* terrainAt(City *city, s32 x, s32 y)
{
	if (!tileExists(city, x, y)) return &invalidTerrain;
	return &city->terrain[tileIndex(city, x, y)];
}

inline Building* getBuildingByID(City *city, u32 buildingID)
{
	if (buildingID <= 0 || buildingID > city->buildingCountMax)
	{
		return null;
	}

	return &(city->buildings[buildingID]);
}

inline Building* getBuildingAtPosition(City *city, V2I position)
{
	return getBuildingByID(city, city->tileBuildings[tileIndex(city,position.x,position.y)]);
}