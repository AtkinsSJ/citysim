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

struct ZoneSector
{
	Rect2I bounds;

	ZoneType *tileZone;
};

struct ZoneLayer
{
	SectorGrid<ZoneSector> sectors;

	ChunkPool<V2I> zoneLocationsChunkPool;

	ChunkedArray<V2I> emptyRZones;
	ChunkedArray<V2I> filledRZones;

	ChunkedArray<V2I> emptyCZones;
	ChunkedArray<V2I> filledCZones;

	ChunkedArray<V2I> emptyIZones;
	ChunkedArray<V2I> filledIZones;
};

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena);
void placeZone(City *city, ZoneType zoneType, Rect2I area);
void markZonesAsEmpty(City *city, Rect2I footprint);
ZoneType getZoneAt(City *city, s32 x, s32 y);
void drawZones(ZoneLayer *zoneLayer, Renderer *renderer, Rect2I visibleArea, s32 shaderID);
