#pragma once

enum DataLayer
{
	DataLayer_None,

	DataLayer_Paths,
	DataLayer_Power,

	DataLayerCount
};

struct PathLayer
{
	s32 pathGroupCount;
	s32 *data; // Represents the pathing 'group'. 0 = unpathable, >0 = any tile with the same value is connected
};

struct PowerGroup
{
	s32 production;
	s32 consumption;
};

struct PowerLayer
{
	PowerGroup combined;
	Array<PowerGroup> groups;
	s32 *data; // Represents the power grid "group". 0 = none, >0 = any tile with the same value is connected
};

struct City
{
	String name;
	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	Terrain *terrain;
	PathLayer pathLayer;
	PowerLayer powerLayer;

	Array<Building> buildings;
	u32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null).

	ZoneType *tileZones; // x,y -> ZoneType

	s32 totalResidents;
	s32 totalJobs;
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

inline Terrain terrainAt(City *city, s32 x, s32 y)
{
	if (!tileExists(city, x, y)) return invalidTerrain;
	return city->terrain[tileIndex(city, x, y)];
}

inline Building* getBuildingByID(City *city, u32 buildingID)
{
	if (buildingID <= 0 || buildingID > city->buildings.count)
	{
		return null;
	}

	return pointerTo(&city->buildings, buildingID);
}

inline Building* getBuildingAtPosition(City *city, V2I position)
{
	return getBuildingByID(city, city->tileBuildings[tileIndex(city,position.x,position.y)]);
}