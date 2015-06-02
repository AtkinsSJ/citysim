
inline City createCity(uint32 width, uint32 height) {
	City city = {};
	city.width = width;
	city.height = height;
	city.terrain = new Terrain[width*height]();
	city.tileBuildings = new uint32[width*height]();
	city.buildingCount = 0;
	city.buildingCountMax = ArrayCount(city.buildings);

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

bool canPlaceBuilding(City &city, BuildingArchetype selectedBuildingArchetype, Coord position) {
	// Check terrain is buildable and empty
	BuildingDefinition def = buildingDefinitions[selectedBuildingArchetype];

	for (int32 y=0; y<def.height; y++) {
		for (int32 x=0; x<def.width; x++) {
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

/**
 * Attempt to place a building. Returns whether successful.
 */
bool placeBuilding(City &city, Building building) {
	if (!canPlaceBuilding(city, building.archetype, building.footprint.pos)) {
		return false;
	}

	if (city.buildingCount >= city.buildingCountMax) {
		return false;
	}

	// Find first free building
	uint32 buildingID = 0;
	for (int i = 0; i < city.buildingCountMax; ++i) {
		if (!city.buildings[i].exists) {
			buildingID = i + 1;
			break;
		}
	}

	if (!buildingID) {
		// We didn't find a free building slot
		return false;
	}

	city.buildingCount++;

	Building *b = getBuildingByID(city, buildingID);
	*b = building;
	for (int16 y=0; y<building.footprint.h; y++) {
		for (int16 x=0; x<building.footprint.w; x++) {
			city.tileBuildings[tileIndex(city,building.footprint.x+x,building.footprint.y+y)] = buildingID;
		}
	}
	return true;
}

bool demolish(City &city, Coord position) {
	if (!tileExists(city, position.x, position.y)) return false;

	uint32 posTI = tileIndex(city, position.x, position.y);

	uint32 buildingID = city.tileBuildings[posTI];
	if (buildingID) {
		// TODO: Demolition cost

		// Clear all references to this building
		Building *building = getBuildingByID(city, buildingID);
		SDL_assert(building);

		for (int32 y = building->footprint.y;
			y < building->footprint.y + building->footprint.h;
			y++) {

			for (int32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++) {

				city.tileBuildings[tileIndex(city, x, y)] = 0;
			}
		}
		// Mark the building as non-existent, then next time we create a new building,
		// we'll find the first non-existent building to replace!
		building->exists = false;
		city.buildingCount--;

		return true;

	} else {
		// TODO: Handle clearing of terrain here.
		return false;
	}
	
}