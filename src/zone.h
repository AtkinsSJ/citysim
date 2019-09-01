#pragma once

struct City;

enum ZoneType
{
	Zone_None,

	Zone_Residential,
	Zone_Commercial,
	Zone_Industrial,

	ZoneCount,
	FirstZoneType = Zone_Residential
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

enum ZoneSectorFlags
{
	ZoneSector_HasResZones      = 1 << 0,
	ZoneSector_HasComZones      = 1 << 1,
	ZoneSector_HasIndZones      = 1 << 2,
	ZoneSector_HasEmptyResZones = 1 << 3,
	ZoneSector_HasEmptyComZones = 1 << 4,
	ZoneSector_HasEmptyIndZones = 1 << 5,
};

struct ZoneSector
{
	Rect2I bounds;
	u32 zoneSectorFlags;
};

struct ZoneLayer
{
	ZoneType *tileZone;

	SectorGrid<ZoneSector> sectors;

	BitArray sectorsWithZones[ZoneCount];
	BitArray sectorsWithEmptyZones[ZoneCount];

	s32 population[ZoneCount]; // NB: Zone_None is used for jobs provided by non-zone, city buildings

	// Calculated every so often
	s32 demand[ZoneCount];
};

struct CanZoneQuery
{
	Rect2I bounds;
	ZoneDef *zoneDef;

	s32 zoneableTilesCount;
	u8 *tileCanBeZoned; // NB: is either 0 or 1
};

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena);
void updateZoneLayer(City *city, ZoneLayer *layer);
void calculateDemand(City *city, ZoneLayer *layer);

ZoneDef getZoneDef(s32 type);

CanZoneQuery *queryCanZoneTiles(City *city, ZoneType zoneType, Rect2I bounds);
bool canZoneTile(CanZoneQuery *query, s32 x, s32 y);
s32 calculateZoneCost(CanZoneQuery *query);

void placeZone(City *city, ZoneType zoneType, Rect2I area);
void markZonesAsEmpty(City *city, Rect2I footprint);
ZoneType getZoneAt(City *city, s32 x, s32 y);

void drawZones(City *city, Rect2I visibleArea, s8 shaderID);

void growSomeZoneBuildings(City *city);
bool isZoneAcceptable(City *city, ZoneType zoneType, s32 x, s32 y);

s32 getTotalResidents(City *city);
s32 getTotalJobs(City *city);
