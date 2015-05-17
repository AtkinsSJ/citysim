#pragma once

enum Terrain {
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Size
};

struct City {
	uint32 width, height;
	Terrain *terrain;

	uint32 buildingCount;
	Building buildings[256]; // TODO: Make the number of buildings unlimited!
	uint32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null), however they're still stored
	// from position 0!
};

inline Building* getBuildingByID(City &city, uint32 buildingID) {
	if (buildingID <= 0 || buildingID > city.buildingCount) return null;

	if (buildingID > city.buildingCount) return null;

	return &(city.buildings[buildingID - 1]);
}

#include "city.cpp"