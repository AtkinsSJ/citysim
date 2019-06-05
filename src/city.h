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

#define SECTOR_SIZE 16
struct Sector
{
	Rect2I bounds;

	// All of these are in [y][x] order!
	Terrain terrain[SECTOR_SIZE][SECTOR_SIZE];
	s32 tileBuildings[SECTOR_SIZE][SECTOR_SIZE]; // Map from y,x -> building id at that location. Building IDs are 1-indexed (0 meaning null).
	ZoneType zones[SECTOR_SIZE][SECTOR_SIZE];
};

struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	s32 sectorsX, sectorsY;
	Sector *sectors;

	PathLayer pathLayer;
	PowerLayer powerLayer;

	u32 highestBuildingID;
	ChunkedArray<Building> buildings;

	struct ZoneLayer zoneLayer;

	s32 totalResidents;
	s32 totalJobs;

	// Calculated every so often
	s32 residentialDemand;
	s32 commercialDemand;
	s32 industrialDemand;
};

inline s32 tileIndex(City *city, s32 x, s32 y)
{
	return (y * city->width) + x;
}

inline Sector *getSector(City *city, s32 sectorX, s32 sectorY)
{
	Sector *result = null;

	if (sectorX >= 0 && sectorX < city->sectorsX && sectorY >= 0 && sectorY < city->sectorsY)
	{
		result = city->sectors + (sectorY * city->sectorsX) + sectorX;
	}

	return result;
}

inline Sector *sectorAtTilePos(City *city, s32 x, s32 y)
{
	return getSector(city, x / SECTOR_SIZE, y / SECTOR_SIZE);
}

inline bool tileExists(City *city, s32 x, s32 y)
{
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Terrain *terrainAt(City *city, s32 x, s32 y)
{
	Terrain *result = &invalidTerrain;
	Sector *sector = sectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		return &sector->terrain[relY][relX];
	}

	return result;
}

inline Building* getBuildingByID(City *city, s32 buildingID)
{
	if (buildingID <= 0 || buildingID > city->buildings.count)
	{
		return null;
	}

	return get(&city->buildings, buildingID);
}

inline s32 getBuildingIDAtPosition(City *city, s32 x, s32 y)
{
	s32 result = 0;
	Sector *sector = sectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->tileBuildings[relY][relX];
	}

	return result;
}

inline Building* getBuildingAtPosition(City *city, s32 x, s32 y)
{
	return getBuildingByID(city, getBuildingIDAtPosition(city, x, y));
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
	Sector *sector = sectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->zones[relY][relX];
	}

	return result;
}
