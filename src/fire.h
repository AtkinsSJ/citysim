#pragma once

struct Fire
{
	Entity *entity;

	V2I pos;
	// TODO: severity
	// TODO: Timing information (fires shouldn't last forever!)
	GameTimestamp startDate;
};

struct FireSector
{
	Rect2I bounds;

	ChunkedArray<Fire> activeFires;
};

struct FireLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<FireSector> sectors;

	u8 maxFireRadius;
	Array2<u16> tileFireProximityEffect;

	Array2<u8> tileTotalFireRisk; // Risks combined

	Array2<u8> tileFireProtection;

	Array2<u8> tileOverallFireRisk; // Risks after we've taken protection into account

	ChunkedArray<BuildingRef> fireProtectionBuildings;

	ArrayChunkPool<Fire> firePool;
	s32 activeFireCount;

	f32 fundingLevel; // @Budget

	// Debug stuff
	V2I debugTileInspectionPos;
};

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena);
void updateFireLayer(City *city, FireLayer *layer);
void markFireLayerDirty(FireLayer *layer, Rect2I bounds);

void notifyNewBuilding(FireLayer *layer, BuildingDef *def, Building *building);
void notifyBuildingDemolished(FireLayer *layer, BuildingDef *def, Building *building);

Indexed<Fire*> findFireAt(City *city, s32 x, s32 y);
bool doesAreaContainFire(City *city, Rect2I bounds);
void startFireAt(City *city, s32 x, s32 y);
void addFireRaw(City *city, s32 x, s32 y, GameTimestamp startDate);
void updateFire(City *city, Fire *fire);
void removeFireAt(City *city, s32 x, s32 y);

u8 getFireRiskAt(City *city, s32 x, s32 y);
u8 getFireProtectionAt(City *city, s32 x, s32 y);
f32 getFireProtectionPercentAt(City *city, s32 x, s32 y);

void debugInspectFire(UI::Panel *panel, City *city, s32 x, s32 y);

void saveFireLayer(FireLayer *layer, struct BinaryFileWriter *writer);
bool loadFireLayer(FireLayer *layer, City *city, struct BinaryFileReader *reader);
