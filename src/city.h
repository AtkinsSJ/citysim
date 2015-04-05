#pragma once

enum Terrain {
	Terrain_Ground,
	Terrain_Water,
	Terrain_Size
};

struct City {
	uint16 width, height;
	Terrain *terrain;
};

#include "city.cpp"