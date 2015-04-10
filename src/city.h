#pragma once

enum Terrain {
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Size
};

struct Building {
	Rect footprint;
};

struct City {
	uint32 width, height;
	Terrain *terrain;

	uint32 buildingCount;
	Building buildings[256]; // TODO: Make the number of buildings unlimited!
};

#include "city.cpp"