#pragma once

enum Terrain {
	Terrain_Ground,
	Terrain_Water,
};

struct City {
	uint16 width, height;
	Terrain *terrain;
};