#pragma once

enum Terrain {
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Size
};

struct Building {
	Coord position;
};

struct City {
	uint16 width, height;
	Terrain *terrain;

	uint16 buildingCount;
	Building buildings[16];
};

#include "city.cpp"