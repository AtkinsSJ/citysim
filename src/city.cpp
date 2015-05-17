
inline City createCity(uint32 width, uint32 height) {
	City city = {};
	city.width = width;
	city.height = height;
	city.terrain = new Terrain[width*height]();
	city.tileBuildings = new Building*[width*height]();

	return city;
}

inline void freeCity(City &city) {
	delete[] city.terrain;
	city = {};
}

inline uint32 tileIndex(City &city, uint32 x, uint32 y) {
	return (y * city.width) + x;
}

inline bool tileExists(City &city, uint32 x, uint32 y) {
	return (x >= 0) && (x < city.width)
		&& (y >= 0) && (y < city.height);
}

inline Terrain terrainAt(City &city, uint32 x, uint32 y) {
	if (!tileExists(city, x, y)) return Terrain_Invalid;
	return city.terrain[tileIndex(city, x, y)];
}

void generateTerrain(City &city) {
	for (uint32 y = 0; y < city.height; y++) {
		for (uint32 x = 0; x < city.width; x++) {
			Terrain t = Terrain_Ground;
			if ((rand() % 100) < 5) {
				t = Terrain_Water;
			}
			city.terrain[tileIndex(city,x,y)] = t;
		}
	}
}

inline bool canPlaceBuilding(City &city, BuildingArchetype selectedBuildingArchetype, Coord position) {
	// Check terrain is buildable and empty
	BuildingDefinition *def = buildingDefinitions + selectedBuildingArchetype;

	for (int32 y=0; y<def->height; y++) {
		for (int32 x=0; x<def->width; x++) {
			uint32 ti = tileIndex(city, position.x + x, position.y + y);
			if (city.terrain[ti] != Terrain_Ground) {
				return false;
			}

			if (city.tileBuildings[ti] != 0) {
				return false;
			}
		}
	}
	return true;
}
/*
inline bool canPlaceBuilding(City &city, Building building) {
	// Check terrain is buildable
	for (int32 y=0; y<building.footprint.h; y++) {
		for (int32 x=0; x<building.footprint.w; x++) {
			uint32 ti = tileIndex(city, building.footprint.x + x, building.footprint.y + y);
			if (city.terrain[ti] != Terrain_Ground) {
				return false;
			}

			if (city.tileBuildings[ti] != 0) {
				return false;
			}
		}
	}
	return true;
}*/

/**
 * Attempt to place a building. Returns whether successful.
 */
inline bool placeBuilding(City &city, Building building) {
	if (!canPlaceBuilding(city, building.archetype, building.footprint.pos)) {
		return false;
	}

	if (city.buildingCount >= ArrayCount(city.buildings)) {
		return false;
	}
	Building *b = &(city.buildings[city.buildingCount++]);
	*b = building;
	for (int16 y=0; y<building.footprint.h; y++) {
		for (int16 x=0; x<building.footprint.w; x++) {
			city.tileBuildings[tileIndex(city,building.footprint.x+x,building.footprint.y+y)] = b;
		}
	}
	return true;
}