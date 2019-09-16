#pragma once

struct CitySector
{
	Rect2I bounds;

	// NB: A building is owned by a CitySector if its top-left corner tile is inside that CitySector.
	ChunkedArray<Building *> ownedBuildings;
};

struct City
{
	String name;

	Random *gameRandom;

	s32 funds;
	s32 monthlyExpenditure;

	Rect2I bounds;

	s32 *tileBuildingIndex; // NB: Index into buildings array, NOT Building.id!
	OccupancyArray<Building> buildings;
	u32 highestBuildingID;
	s32 nextBuildingSectorUpdateIndex;

	SectorGrid<CitySector> sectors;

	FireLayer      fireLayer;
	HealthLayer    healthLayer;
	LandValueLayer landValueLayer;
	PollutionLayer pollutionLayer;
	PowerLayer     powerLayer;
	TerrainLayer   terrainLayer;
	TransportLayer transportLayer;
	ZoneLayer      zoneLayer;

	ArrayChunkPool<Building *>  sectorBuildingsChunkPool;
	ArrayChunkPool<Rect2I>      sectorBoundariesChunkPool;
	ArrayChunkPool<BuildingRef> buildingRefsChunkPool;
};

const u8 maxDistanceToWater = 10;

//
// Public API
//
void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds);
void drawCity(City *city, Rect2I visibleTileBounds, Rect2I demolitionRect);

void markAreaDirty(City *city, Rect2I bounds);

bool tileExists(City *city, s32 x, s32 y);
template<typename T>
T *getTile(City *city, T *tiles, s32 x, s32 y);
template<typename T>
inline T getTileValue(City *city, T *tiles, s32 x, s32 y);
template<typename T>
T getTileValueIfExists(City *city, T *tiles, s32 x, s32 y, T defaultValue);
template<typename T>
void setTile(City *city, T *tiles, s32 x, s32 y, T value);

bool canAfford(City *city, s32 cost);
void spend(City *city, s32 cost);

bool buildingExistsAt(City *city, s32 x, s32 y);
Building* getBuildingAt(City *city, s32 x, s32 y);
// Returns a TEMPORARY-allocated list of buildings that are overlapping `area`, guaranteeing that
// each building is only listed once. No guarantees are made about the order.
enum BuildingQueryFlags
{
	BQF_RequireOriginInArea = 1 << 0, // Only return buildings whose origin (top-left corner) is within the given area.
};
ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area, u32 flags=0);
bool canPlaceBuilding(City *city, BuildingDef *def, s32 left, s32 top);
s32 calculateBuildCost(City *city, BuildingDef *def, Rect2I area);
void placeBuilding(City *city, BuildingDef *def, s32 left, s32 top, bool markAreasDirty=true);
void placeBuildingRect(City *city, BuildingDef *def, Rect2I area);
void drawBuildings(City *city, Rect2I visibleTileBounds, s8 shaderID, Rect2I demolitionRect);
void updateSomeBuildings(City *city);

s32 calculateDemolitionCost(City *city, Rect2I area);
void demolishRect(City *city, Rect2I area);

//
// Private API
//
Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint);
