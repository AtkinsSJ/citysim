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


#define SECTOR_SIZE 16
struct Sector
{
	Rect2I bounds;

	// All of these are in [y][x] order!
	Terrain terrain[SECTOR_SIZE][SECTOR_SIZE];
	TileBuildingRef tileBuilding[SECTOR_SIZE][SECTOR_SIZE];
	ZoneType tileZone[SECTOR_SIZE][SECTOR_SIZE];
	s32 tilePathGroup[SECTOR_SIZE][SECTOR_SIZE]; // 0 = unpathable, >0 = any tile with the same value is connected

	// 0 = none, >0 = any tile with the same value is connected
	// POWER_GROUP_UNKNOWN is used as a temporary value while recalculating
	u8 tilePowerGroup[SECTOR_SIZE][SECTOR_SIZE];

	// NB: A building is owned by a Sector if its top-left corner tile is inside that Sector.
	ChunkedArray<Building>   buildings;
	// NB: Power groups start at 1, (0 means "none") so subtract 1 from the value in tilePowerGroup to get the index!
	ChunkedArray<PowerGroup> powerGroups;
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
	area = cropRectangle(area, irectXYWH(0, 0, city->width, city->height));

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
	Sector *result = null;

	if (sectorX >= 0 && sectorX < city->sectorsX && sectorY >= 0 && sectorY < city->sectorsY)
	{
		result = city->sectors + (sectorY * city->sectorsX) + sectorX;
	}

	return result;
}

inline Sector *getSectorAtTilePos(City *city, s32 x, s32 y)
{
	Sector *result = null;

	if (tileExists(city, x, y))
	{
		result = getSector(city, x / SECTOR_SIZE, y / SECTOR_SIZE);
	}

	return result;
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

// Returns a TEMPORARY-allocated list of buildings that are overlapping `area`, guaranteeing that
// each building is only listed once. No guarantees are made about the order.
ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area);

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

inline PowerGroup *getPowerGroupAt(City *city, s32 x, s32 y)
{
	PowerGroup *result = null;
	Sector *sector = getSectorAtTilePos(city, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		s32 powerGroupIndex = sector->tilePowerGroup[relY][relX];
		if (powerGroupIndex != 0)
		{
			result = get(&sector->powerGroups, powerGroupIndex - 1);
		}
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

inline Rect2I cropRectangleToRelativeWithinSector(Rect2I area, Sector *sector)
{
	Rect2I result = cropRectangle(area, sector->bounds);
	result.x -= sector->bounds.x;
	result.y -= sector->bounds.y;

	return result;
}
