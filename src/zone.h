#pragma once

struct City;

enum ZoneType
{
	Zone_None,

	Zone_Residential,
	Zone_Commercial,
	Zone_Industrial,

	ZoneCount
};

struct ZoneDef
{
	ZoneType typeID;
	String name;
	V4 color;
	s32 costPerTile;
	bool carriesPower;
	s32 maximumDistanceToRoad;
};

ZoneDef zoneDefs[] = {
	{Zone_None,        makeString("Dezone"),      color255(255, 255, 255, 128), 10, false, 0},
	{Zone_Residential, makeString("Residential"), color255(  0, 255,   0, 128), 10, true,  3},
	{Zone_Commercial,  makeString("Commercial"),  color255(  0,   0, 255, 128), 10, true,  2},
	{Zone_Industrial,  makeString("Industrial"),  color255(255, 255,   0, 128), 20, true,  4},
};

struct ZoneLayer
{
	ChunkPool<V2I> zoneLocationsChunkPool;

	ChunkedArray<s32> rGrowableBuildings;
	ChunkedArray<V2I> emptyRZones;
	ChunkedArray<V2I> filledRZones;
	s32 maxRBuildingDim;

	ChunkedArray<s32> cGrowableBuildings;
	ChunkedArray<V2I> emptyCZones;
	ChunkedArray<V2I> filledCZones;
	s32 maxCBuildingDim;

	ChunkedArray<s32> iGrowableBuildings;
	ChunkedArray<V2I> emptyIZones;
	ChunkedArray<V2I> filledIZones;
	s32 maxIBuildingDim;
};

void initZoneLayer(MemoryArena *memoryArena, ZoneLayer *zoneLayer);
void placeZone(UIState *uiState, City *city, ZoneType zoneType, Rect2I area, bool chargeMoney=true);
void markZonesAsEmpty(City *city, Rect2I footprint);
