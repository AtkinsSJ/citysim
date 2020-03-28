#pragma once

struct CrimeLayer
{
	DirtyRects dirtyRects;
	
	SectorGrid<BasicSector> sectors;

	Array2<u8> tilePoliceCoverage;

	ChunkedArray<BuildingRef> policeBuildings;
	s32 totalJailCapacity;
	s32 occupiedJailCapacity;
	
	f32 fundingLevel; // @Budget
};

void initCrimeLayer(CrimeLayer *layer, City *city, MemoryArena *gameArena);
void updateCrimeLayer(City *city, CrimeLayer *layer);

void registerPoliceBuilding(CrimeLayer *layer, Building *building);
void unregisterPoliceBuilding(CrimeLayer *layer, Building *building);

f32 getPoliceCoveragePercentAt(City *city, s32 x, s32 y);
