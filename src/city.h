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


struct TileBuildingRef
{
	bool isOccupied;

	s32 originX;
	s32 originY;

	// TODO: Could combine these two, have -1 be for non-local buildings?
	bool isLocal;
	s32 localIndex;
};

struct Sector;
struct City;

template<typename SectorType>
struct SectorGrid
{
	s32 width, height; // world size
	s32 sectorSize;
	s32 sectorsX, sectorsY;

	s32 count;
	SectorType *sectors;
};

template<typename SectorType>
void initSectorGrid(SectorGrid<SectorType> *grid, MemoryArena *arena, s32 cityWidth, s32 cityHeight, s32 sectorSize);

template<typename SectorType>
inline SectorType *getSector(SectorGrid<SectorType> *grid, s32 sectorX, s32 sectorY);

template<typename SectorType>
inline SectorType *getSectorAtTilePos(SectorGrid<SectorType> *grid, s32 x, s32 y);

#define SECTOR_SIZE 16
#include "building.h"
#include "power.h"
#include "terrain.h"
#include "zone.h"

struct Sector
{
	Rect2I bounds;

	// All of these are in [y][x] order!
	Terrain terrain[SECTOR_SIZE][SECTOR_SIZE];
	TileBuildingRef tileBuilding[SECTOR_SIZE][SECTOR_SIZE];
	ZoneType tileZone[SECTOR_SIZE][SECTOR_SIZE];
	s32 tilePathGroup[SECTOR_SIZE][SECTOR_SIZE]; // 0 = unpathable, >0 = any tile with the same value is connected

	// NB: A building is owned by a Sector if its top-left corner tile is inside that Sector.
	ChunkedArray<Building>   buildings;
};


struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	SectorGrid<Sector> sectors;

	PathLayer pathLayer;
	PowerLayer powerLayer;

	u32 highestBuildingID;

	struct ZoneLayer zoneLayer;

	ChunkPool<Building>   sectorBuildingsChunkPool;
	ChunkPool<Rect2I>     sectorBoundariesChunkPool;

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

inline Rect2I getSectorsCovered(City *city, Rect2I area)
{
	area = intersect(area, irectXYWH(0, 0, city->width, city->height));

	Rect2I result = irectMinMax(
		area.x / SECTOR_SIZE,
		area.y / SECTOR_SIZE,

		(area.x + area.w) / SECTOR_SIZE,
		(area.y + area.h) / SECTOR_SIZE
	);

	return result;
}

inline bool tileExists(City *city, s32 x, s32 y)
{
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Sector *getSector(City *city, s32 sectorX, s32 sectorY)
{
	return getSector(&city->sectors, sectorX, sectorY);
}

inline s32 getSectorIndexAtTilePos(s32 x, s32 y, s32 sectorsX)
{
	return (sectorsX * (y / SECTOR_SIZE)) + (x / SECTOR_SIZE);
}

inline Sector *getSectorAtTilePos(City *city, s32 x, s32 y)
{
	return getSectorAtTilePos(&city->sectors, x, y);
}

inline Terrain *terrainAt(City *city, s32 x, s32 y)
{
	Terrain *result = &invalidTerrain;
	Sector *sector = getSectorAtTilePos(&city->sectors, x, y);

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

	Sector *sector = getSectorAtTilePos(&city->sectors, x, y);
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

enum BuildingQueryFlags
{
	BQF_RequireOriginInArea = 1 << 0, // Only return buildings whose origin (top-left corner) is within the given area.
};
// Returns a TEMPORARY-allocated list of buildings that are overlapping `area`, guaranteeing that
// each building is only listed once. No guarantees are made about the order.
// Flags are BuildingQueryFlags
ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area, u32 flags=0);

inline s32 pathGroupAt(City *city, s32 x, s32 y)
{
	s32 result = 0;
	Sector *sector = getSectorAtTilePos(&city->sectors, x, y);

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

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;
	Sector *sector = getSectorAtTilePos(&city->sectors, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = sector->tileZone[relY][relX];
	}

	return result;
}
