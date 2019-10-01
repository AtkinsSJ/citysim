#pragma once

struct HealthLayer
{
	DirtyRects dirtyRects;

	SectorGrid<BasicSector> sectors;

	u8 *tileHealthCoverage;

	ChunkedArray<BuildingRef> healthBuildings;
	
	f32 fundingLevel; // @Budget
};

void initHealthLayer(HealthLayer *layer, City *city, MemoryArena *gameArena);
void updateHealthLayer(City *city, HealthLayer *layer);
void markHealthLayerDirty(HealthLayer *layer, Rect2I bounds);
void drawHealthDataLayer(City *city, Rect2I visibleTileBounds);

void registerHealthBuilding(HealthLayer *layer, Building *building);
void unregisterHealthBuilding(HealthLayer *layer, Building *building);

f32 getHealthCoveragePercentAt(City *city, s32 x, s32 y);
