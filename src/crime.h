#pragma once

struct CrimeLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<BasicSector> sectors;

	u8 *tilePoliceCoverage;

	ChunkedArray<BuildingRef> policeBuildings;
	s32 totalJailCapacity;
	s32 occupiedJailCapacity;
};

void initCrimeLayer(CrimeLayer *layer, City *city, MemoryArena *gameArena);
void updateCrimeLayer(City *city, CrimeLayer *layer);
void drawCrimeDataLayer(City *city, Rect2I visibleTileBounds);

void registerPoliceBuilding(CrimeLayer *layer, Building *building);
void unregisterPoliceBuilding(CrimeLayer *layer, Building *building);
