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
	Building **tileBuildings; // Map from x,y -> building pointer at that location
};

#include "city.cpp"