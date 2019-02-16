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
	ChunkedArray<PowerGroup> groups;
	s32 *data; // Represents the power grid "group". 0 = none, >0 = any tile with the same value is connected
};

struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	Terrain *terrain;
	PathLayer pathLayer;
	PowerLayer powerLayer;

	ChunkedArray<Building> buildings;
	s32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null).

	struct ZoneLayer zoneLayer;

	s32 totalResidents;
	s32 totalJobs;

	// Calculated every so often
	s32 residentialDemand;
	s32 commercialDemand;
	s32 industrialDemand;
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

inline Building* getBuildingByID(City *city, s32 buildingID)
{
	if (buildingID <= 0 || buildingID > city->buildings.count)
	{
		return null;
	}

	return get(&city->buildings, buildingID);
}

inline Building* getBuildingAtPosition(City *city, s32 x, s32 y)
{
	if (!tileExists(city, x, y))
	{
		return null;
	}
	return getBuildingByID(city, city->tileBuildings[tileIndex(city, x, y)]);
}

inline s32 pathGroupAt(City *city, s32 x, s32 y)
{
	s32 result = 0;

	if (tileExists(city, x, y))
	{
		result = city->pathLayer.data[tileIndex(city, x, y)];
	}

	return result;
}

inline bool isPathable(City *city, s32 x, s32 y)
{
	return pathGroupAt(city, x, y) > 0;
}

inline s32 powerGroupAt(City *city, s32 x, s32 y)
{
	s32 result = 0;

	if (tileExists(city, x, y))
	{
		result = city->powerLayer.data[tileIndex(city, x, y)];
	}

	return result;
}

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;

	if (tileExists(city, x, y))
	{
		result = city->zoneLayer.tiles[tileIndex(city, x, y)];
	}

	return result;
}