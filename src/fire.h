#pragma once

struct Fire
{
	V2I pos;
};

struct FireLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<BasicSector> sectors;

	u8 maxFireRadius;
	u16 *tileFireProximityEffect;

	u8 *tileTotalFireRisk; // Risks combined

	u8 *tileFireProtection;

	u8 *tileOverallFireRisk; // Risks after we've taken protection into account

	ChunkedArray<BuildingRef> fireProtectionBuildings;

	ChunkedArray<Fire> activeFires;
};

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena);
void updateFireLayer(City *city, FireLayer *layer);
void markFireLayerDirty(FireLayer *layer, Rect2I bounds);
void drawFireDataLayer(City *city, Rect2I visibleTileBounds);
void drawFires(City *city, Rect2I visibleTileBounds);

void registerFireProtectionBuilding(FireLayer *layer, Building *building);
void unregisterFireProtectionBuilding(FireLayer *layer, Building *building);

void startFireAt(City *city, s32 x, s32 y);

u8 getFireRiskAt(City *city, s32 x, s32 y);
u8 getFireProtectionAt(City *city, s32 x, s32 y);
f32 getFireProtectionPercentAt(City *city, s32 x, s32 y);
