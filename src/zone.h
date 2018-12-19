#pragma once

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
};

ZoneDef zoneDefs[] = {
	{Zone_None,        stringFromChars("Dezone"),      color255(255, 255, 255, 128), 10, false},
	{Zone_Residential, stringFromChars("Residential"), color255(  0, 255,   0, 128), 10, true},
	{Zone_Commercial,  stringFromChars("Commercial"),  color255(  0,   0, 255, 128), 10, true},
	{Zone_Industrial,  stringFromChars("Industrial"),  color255(255, 255,   0, 128), 20, true},
};

struct ZoneLayer
{
	ChunkedArray<V2I> emptyRZones;
	ChunkedArray<V2I> filledRZones;

	ChunkedArray<V2I> emptyCZones;
	ChunkedArray<V2I> filledCZones;

	ChunkedArray<V2I> emptyIZones;
	ChunkedArray<V2I> filledIZones;

	ZoneType *tiles; // x,y -> ZoneType
};

void initZoneLayer(MemoryArena *memoryArena, ZoneLayer *zoneLayer, s32 tileCount);
void placeZone(UIState *uiState, struct City *city, ZoneType zoneType, Rect2I area, bool chargeMoney=true);