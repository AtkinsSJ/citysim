#pragma once

struct FireLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<BasicSector> sectors;

	u8 *tileBuildingFireRisk;
	u8 *tileTotalFireRisk; // Risks combined
	u8 *tileFireProtection;
	u8 *tileOverallFireRisk; // Risks after we've taken protection into account

	ChunkedArray<BuildingRef> fireProtectionBuildings;
};

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena);
void updateFireLayer(City *city, FireLayer *layer);
void markFireLayerDirty(FireLayer *layer, Rect2I bounds);
void drawFireRiskDataLayer(City *city, Rect2I visibleTileBounds);

void registerFireProtectionBuilding(FireLayer *layer, Building *building);
void unregisterFireProtectionBuilding(FireLayer *layer, Building *building);

u8 getFireRiskAt(City *city, s32 x, s32 y);
u8 getFireProtectionAt(City *city, s32 x, s32 y);
f32 getFireProtectionPercentAt(City *city, s32 x, s32 y);
