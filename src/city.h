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
	String playerName;

	s32 funds;
	s32 monthlyExpenditure;

	Rect2I bounds;

	Array2<s32> tileBuildingIndex; // NB: Index into buildings array, NOT Building.id!
	OccupancyArray<Building> buildings;
	u32 highestBuildingID;

	SectorGrid<CitySector> sectors;

	OccupancyArray<Entity> entities;

	BudgetLayer    budgetLayer;

	CrimeLayer     crimeLayer;
	EducationLayer educationLayer;
	FireLayer      fireLayer;
	HealthLayer    healthLayer;
	LandValueLayer landValueLayer;
	PollutionLayer pollutionLayer;
	PowerLayer     powerLayer;
	TerrainLayer   terrainLayer;
	TransportLayer transportLayer;
	ZoneLayer      zoneLayer;

	Rect2I demolitionRect;

	ArrayChunkPool<Building *>  sectorBuildingsChunkPool;
	ArrayChunkPool<Rect2I>      sectorBoundariesChunkPool;
	ArrayChunkPool<BuildingRef> buildingRefsChunkPool;
};

const u8 maxDistanceToWater = 10;

//
// Public API
//
void initCity(MemoryArena *gameArena, City *city, u32 width, u32 height, String name, String playerName, s32 funds);
void drawCity(City *city, Rect2I visibleTileBounds);

void markAreaDirty(City *city, Rect2I bounds);

bool tileExists(City *city, s32 x, s32 y);

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
void updateSomeBuildings(City *city);

s32 calculateDemolitionCost(City *city, Rect2I area);
void demolishRect(City *city, Rect2I area);

template <typename T>
Entity *addEntity(City *city, EntityType type, T *entityData);
void removeEntity(City *city, Entity *entity);
void drawEntities(City *city, Rect2I visibleTileBounds);

void saveBuildings(City *city, struct BinaryFileWriter *writer);
bool loadBuildings(City *city, struct BinaryFileReader *reader);

//
// Private API
//
Building *addBuildingDirect(City *city, s32 id, BuildingDef *def, Rect2I footprint, GameTimestamp creationDate);
Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint, GameTimestamp creationDate = getCurrentTimestamp());
