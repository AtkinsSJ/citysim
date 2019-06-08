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
};

struct TileBuildingRef
{
	bool isOccupied;

	s32 originX;
	s32 originY;

	// TODO: Could combine these two, have -1 be for non-local buildings?
	bool isLocal;
	s32 localIndex;
};

enum SectorFlags
{

};

#define SECTOR_SIZE 16
struct Sector
{
	Rect2I bounds;
	u32 flags;

	// All of these are in [y][x] order!
	Terrain terrain[SECTOR_SIZE][SECTOR_SIZE];
	TileBuildingRef tileBuilding[SECTOR_SIZE][SECTOR_SIZE];
	ZoneType tileZone[SECTOR_SIZE][SECTOR_SIZE];
	s32 tilePathGroup[SECTOR_SIZE][SECTOR_SIZE]; // 0 = unpathable, >0 = any tile with the same value is connected
	s32 tilePowerGroup[SECTOR_SIZE][SECTOR_SIZE]; // 0 = none, >0 = any tile with the same value is connected

	ChunkedArray<Building> buildings; // A building is owned by a Sector if its top-left corner tile is inside that Sector.
};

struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	s32 sectorsX, sectorsY;
	s32 sectorCount;
	Sector *sectors;

	PathLayer pathLayer;
	PowerLayer powerLayer;

	u32 highestBuildingID;

	struct ZoneLayer zoneLayer;

	ChunkPool<Building> sectorBuildingsChunkPool;

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

inline Sector *getSectorAtTilePos(City *city, s32 x, s32 y)
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
	Sector *sector = getSectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		return &sector->terrain[relY][relX];
	}

	return result;
}

TileBuildingRef *getSectorBuildingRefAtWorldPosition(Sector *sector, s32 x, s32 y)
{
	ASSERT(contains(sector->bounds, x, y), "getSectorBuildingRefAtWorldPosition() passed a coordinate that is outside of the sector!");

	s32 relX = x - sector->bounds.x;
	s32 relY = y - sector->bounds.y;

	return &sector->tileBuilding[relY][relX];
}

Building* getBuildingAtPosition(City *city, s32 x, s32 y)
{
	Building *result = null;

	Sector *sector = getSectorAtTilePos(city, x, y);
	if (sector != null)
	{
		TileBuildingRef *ref = getSectorBuildingRefAtWorldPosition(sector, x, y);
		if (ref->isOccupied)
		{
			if (ref->isLocal)
			{
				result = get(&sector->buildings, ref->localIndex);
			}
			else
			{
				// Decided that recursion is the easiest option here to avoid a whole load of
				// duplicate code with slightly different variable names. (Which sounds like a
				// recipe for a whole load of subtle bugs.)
				// We SHOULD only be recursing once, if it's more than once that's a bug. But IDK.
				// - Sam, 05/06/2019
				result = getBuildingAtPosition(city, ref->originX, ref->originY);
			}
		}
	}

	return result;
}

inline s32 pathGroupAt(City *city, s32 x, s32 y)
{
	s32 result = 0;
	Sector *sector = getSectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->tilePathGroup[relY][relX];
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
	Sector *sector = getSectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->tilePowerGroup[relY][relX];
	}

	return result;
}

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;
	Sector *sector = getSectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->tileZone[relY][relX];
	}

	return result;
}

Rect2I cropRectangleToRelativeWithinSector(Rect2I area, Sector *sector)
{
	Rect2I result = cropRectangle(area, sector->bounds);
	result.x -= sector->bounds.x;
	result.y -= sector->bounds.y;

	return result;
}
