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
	Terrain *terrain;
	SectorGrid<CitySector> sectors;

	PathLayer pathLayer;
	PowerLayer powerLayer;
	ZoneLayer zoneLayer;

	ArrayChunkPool<Building>   sectorBuildingsChunkPool;
	ArrayChunkPool<Rect2I>     sectorBoundariesChunkPool;

	u32 highestBuildingID;
};

//
// Public API
//
void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds);
void drawCity(City *city, Rect2I visibleTileBounds, Rect2I demolitionRect);

void generateTerrain(City *city);
void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID);
Terrain *getTerrainAt(City *city, s32 x, s32 y);

bool tileExists(City *city, s32 x, s32 y);
template<typename T>
T *getTile(City *city, T *tiles, s32 x, s32 y);
template<typename T>
void setTile(City *city, T *tiles, s32 x, s32 y, T value);

bool canAfford(City *city, s32 cost);
void spend(City *city, s32 cost);

bool buildingExistsAtPosition(City *city, s32 x, s32 y);
Building* getBuildingAtPosition(City *city, s32 x, s32 y);
// Returns a TEMPORARY-allocated list of buildings that are overlapping `area`, guaranteeing that
// each building is only listed once. No guarantees are made about the order.
enum BuildingQueryFlags
{
	BQF_RequireOriginInArea = 1 << 0, // Only return buildings whose origin (top-left corner) is within the given area.
};
ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area, u32 flags=0);
bool canPlaceBuilding(City *city, BuildingDef *def, s32 left, s32 top);
s32 calculateBuildCost(City *city, BuildingDef *def, Rect2I area);
void placeBuilding(City *city, BuildingDef *def, s32 left, s32 top);
void placeBuildingRect(City *city, BuildingDef *def, Rect2I area);
void drawBuildings(City *city, Rect2I visibleTileBounds, s8 shaderID, Rect2I demolitionRect);

s32 calculateDemolitionCost(City *city, Rect2I area);
void demolishRect(City *city, Rect2I area);

// Pathing-related stuff, which will be moved out into a TransportLayer at some point
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck);
s32 getPathGroupAt(City *city, s32 x, s32 y);
bool isPathable(City *city, s32 x, s32 y);

//
// Private API
//
Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint);
TileBuildingRef *getSectorBuildingRefAtWorldPosition(CitySector *sector, s32 x, s32 y);
