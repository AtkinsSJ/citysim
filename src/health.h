#pragma once

struct HealthLayer
{
	DirtyRects dirtyRects;

	SectorGrid<BasicSector> sectors;
	s32 nextSectorUpdateIndex;
	s32 sectorsToUpdatePerTick;

	u8 *tileHealthCoverage;

	ChunkedArray<BuildingRef> healthBuildings;
};

void initHealthLayer(HealthLayer *layer, City *city, MemoryArena *gameArena);
void updateHealthLayer(City *city, HealthLayer *layer);
void markHealthLayerDirty(HealthLayer *layer, Rect2I bounds);
void drawHealthDataLayer(City *city, Rect2I visibleTileBounds);

void registerHealthBuilding(HealthLayer *layer, Building *building);
void unregisterHealthBuilding(HealthLayer *layer, Building *building);
