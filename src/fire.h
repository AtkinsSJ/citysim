#pragma once

struct FireLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<BasicSector> sectors;
	s32 nextSectorUpdateIndex;
	s32 sectorsToUpdatePerTick;

	u8 *tileBuildingFireRisk;
	u8 *tileFireProtection;
	u8 *tileOverallFireRisk;

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
