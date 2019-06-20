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

struct CitySector
{
	Rect2I bounds;

	Terrain         *terrain;
	TileBuildingRef *tileBuilding;
	s32             *tilePathGroup; // 0 = unpathable, >0 = any tile with the same value is connected

	// NB: A building is owned by a CitySector if its top-left corner tile is inside that CitySector.
	ChunkedArray<Building>   buildings;
};

struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	s32 width, height;
	SectorGrid<CitySector> sectors;

	PathLayer pathLayer;
	PowerLayer powerLayer;
	ZoneLayer zoneLayer;

	ChunkPool<Building>   sectorBuildingsChunkPool;
	ChunkPool<Rect2I>     sectorBoundariesChunkPool;

	u32 highestBuildingID;

	s32 totalResidents;
	s32 totalJobs;

	// Calculated every so often
	s32 residentialDemand;
	s32 commercialDemand;
	s32 industrialDemand;
};

inline Rect2I getSectorsCovered(City *city, Rect2I area)
{
	return getSectorsCovered(&city->sectors, area);
}

inline bool tileExists(City *city, s32 x, s32 y)
{
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline CitySector *getSector(City *city, s32 sectorX, s32 sectorY)
{
	return getSector(&city->sectors, sectorX, sectorY);
}

inline CitySector *getSectorAtTilePos(City *city, s32 x, s32 y)
{
	return getSectorAtTilePos(&city->sectors, x, y);
}

inline Terrain *getTerrainAt(City *city, s32 x, s32 y)
{
	Terrain *result = &invalidTerrain;
	CitySector *sector = getSectorAtTilePos(&city->sectors, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		return getSectorTile(sector, sector->terrain, relX, relY);
	}

	return result;
}

TileBuildingRef *getSectorBuildingRefAtWorldPosition(CitySector *sector, s32 x, s32 y)
{
	ASSERT(contains(sector->bounds, x, y), "getSectorBuildingRefAtWorldPosition() passed a coordinate that is outside of the sector!");

	s32 relX = x - sector->bounds.x;
	s32 relY = y - sector->bounds.y;

	return getSectorTile(sector, sector->tileBuilding, relX, relY);
}

Building* getBuildingAtPosition(City *city, s32 x, s32 y)
{
	Building *result = null;

	CitySector *sector = getSectorAtTilePos(&city->sectors, x, y);
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
	CitySector *sector = getSectorAtTilePos(&city->sectors, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = *getSectorTile(sector, sector->tilePathGroup, relX, relY);
	}

	return result;
}

inline bool isPathable(City *city, s32 x, s32 y)
{
	return pathGroupAt(city, x, y) > 0;
}
