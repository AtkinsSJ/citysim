
inline City createCity(uint32 width, uint32 height) {
	City city = {};
	city.width = width;
	city.height = height;
	city.terrain = new Terrain[width*height]();

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
			if ((rand() % 100) < 10) {
				t = Terrain_Water;
			}
			city.terrain[tileIndex(city,x,y)] = t;
		}
	}
}

inline bool canPlaceBuilding(City &city, Building building) {
	// Check terrain is buildable
	for (int32 y=0; y<building.footprint.h; y++) {
		for (int32 x=0; x<building.footprint.w; x++) {
			if (terrainAt(city, building.footprint.x + x, building.footprint.y + y) != Terrain_Ground) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Attempt to place a building. Returns whether successful.
 */
inline bool placeBuilding(City &city, Building building) {
	if (!canPlaceBuilding(city, building)) {
		return false;
	}

	if (city.buildingCount >= ArrayCount(city.buildings)) {
		return false;
	}
	city.buildings[city.buildingCount++] = building;
	return true;
}