#pragma once

struct City;

enum ZoneType : u8
{
	Zone_None = 0,

	Zone_Residential,
	Zone_Commercial,
	Zone_Industrial,

	ZoneTypeCount,
	FirstZoneType = Zone_Residential
};

struct ZoneDef
{
	ZoneType typeID;
	String textAssetName;
	V4 color;
	s32 costPerTile;
	bool carriesPower;
	s32 maximumDistanceToRoad;
};

ZoneDef zoneDefs[] = {
	{Zone_None,        "zone_none"_s,        color255(255, 255, 255, 128), 10, false, 0},
	{Zone_Residential, "zone_residential"_s, color255(  0, 255,   0, 128), 10, true,  3},
	{Zone_Commercial,  "zone_commercial"_s,  color255(  0,   0, 255, 128), 10, true,  2},
	{Zone_Industrial,  "zone_industrial"_s,  color255(255, 255,   0, 128), 20, true,  4},
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

	f32 averageDesirability[ZoneTypeCount];
};

struct ZoneLayer
{
	Array2<u8> tileZone;
	Array2<u8> tileDesirability[ZoneTypeCount];

	SectorGrid<ZoneSector> sectors;

	BitArray sectorsWithZones[ZoneTypeCount];
	BitArray sectorsWithEmptyZones[ZoneTypeCount];

	Array<s32> mostDesirableSectors[ZoneTypeCount];

	s32 population[ZoneTypeCount]; // NB: Zone_None is used for jobs provided by non-zone, city buildings

	// Calculated every so often
	s32 demand[ZoneTypeCount];
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

ZoneDef getZoneDef(s32 zoneType);

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

void saveZoneLayer(ZoneLayer *layer, struct BinaryFileWriter *writer);
bool loadZoneLayer(ZoneLayer *layer, City *city, struct BinaryFileReader *reader);