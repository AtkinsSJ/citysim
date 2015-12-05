
inline void initCity(City *city, uint32 width, uint32 height, char *name, int32 funds) {
	*city = {};
	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;
	city->terrain = new Terrain[width*height]();
	city->tileBuildings = new uint32[width*height]();
	city->buildingCount = 0;
	city->buildingCountMax = ArrayCount(city->buildings);

	city->farmhouse = null;
}

inline void freeCity(City *city) {
	delete[] city->terrain;
	delete[] city->tileBuildings;
	*city = {};
}

inline uint32 tileIndex(City *city, int32 x, int32 y) {
	return (y * city->width) + x;
}

inline bool tileExists(City *city, int32 x, int32 y) {
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Terrain terrainAt(City *city, int32 x, int32 y) {
	if (!tileExists(city, x, y)) return Terrain_Invalid;
	return city->terrain[tileIndex(city, x, y)];
}

inline Building* getBuildingByID(City *city, uint32 buildingID) {
	if (buildingID <= 0 || buildingID > city->buildingCountMax) {
		return null;
	}

	return &(city->buildings[buildingID - 1]);
}

inline Building* getBuildingAtPosition(City *city, Coord position) {
	return getBuildingByID(city, city->tileBuildings[tileIndex(city,position.x,position.y)]);
}

void generateTerrain(City *city) {
	for (int32 y = 0; y < city->height; y++) {
		for (int32 x = 0; x < city->width; x++) {

			real32 px = (real32)x * 0.1f;
			real32 py = (real32)y * 0.1f;

			real32 perlinValue = stb_perlin_noise3(px, py, 0);

			city->terrain[tileIndex(city, x, y)] = (perlinValue > 0.1f)
				? Terrain_Forest
				: Terrain_Ground;
		}
	}
}

bool canAfford(City *city, int32 cost) {
	return city->funds >= cost;
}

void spend(City *city, int32 cost) {
	city->funds -= cost;
}

bool canPlaceBuilding(City *city, BuildingArchetype selectedBuildingArchetype, Coord position,
	bool isAttemptingToBuild = false) {

	// Only allow one farmhouse!
	if (selectedBuildingArchetype == BA_Farmhouse && city->farmhouse) {
		if (isAttemptingToBuild) {
			pushUiMessage("You can only have one farmhouse!");
		}
		return false;
	}

	BuildingDefinition def = buildingDefinitions[selectedBuildingArchetype];

	// Can we afford to build this?
	if (!canAfford(city, def.buildCost)) {
		if (isAttemptingToBuild) {
			pushUiMessage("Not enough money to build this.");
		}
		return false;
	}

	Rect footprint = irectCentreWH(position, def.width, def.height);

	// Are we in bounds?
	if (!rectInRect({0,0, city->width, city->height}, footprint)) {
		if (isAttemptingToBuild) {
			pushUiMessage("You cannot build there.");
		}
		return false;
	}

	// Check terrain is buildable and empty
	for (int32 y=0; y<def.height; y++) {
		for (int32 x=0; x<def.width; x++) {
			uint32 ti = tileIndex(city, footprint.x + x, footprint.y + y);
			if (city->terrain[ti] != Terrain_Ground) {
				if (isAttemptingToBuild) {
					pushUiMessage("You cannot build there.");
				}
				return false;
			}

			if (city->tileBuildings[ti] != 0) {
				if (isAttemptingToBuild) {
					pushUiMessage("You cannot overlap buildings.");
				}
				return false;
			}
		}
	}
	return true;
}

/**
 * Attempt to place a building. Returns whether successful.
 */
bool placeBuilding(City *city, BuildingArchetype archetype, Coord position) {

	if (!canPlaceBuilding(city, archetype, position, true)) {
		return false;
	}

	ASSERT(city->buildingCount < city->buildingCountMax, "City.buildings is full!");

	// Find first free building
	uint32 buildingID = 0;
	for (uint32 i = 0; i < city->buildingCountMax; ++i) {
		if (!city->buildings[i].exists) {
			buildingID = i + 1;
			break;
		}
	}

	ASSERT(buildingID, "No free building! Means that the buildingCount is wrong!");

	city->buildingCount++;

	Building *building = getBuildingByID(city, buildingID);
	BuildingDefinition *def = buildingDefinitions + archetype;

	spend(city, def->buildCost);

	*building = {};
	building->exists = true;
	building->archetype = archetype;
	building->footprint = irectCentreWH(position, def->width, def->height);

	// Building-specific stuff
	switch (archetype) {
		case BA_Farmhouse: {
			city->farmhouse = building;
		} break;

		case BA_Barn: {
			city->barns[city->barnCount++] = building;
		} break;
	}

	for (int16 y=0; y<building->footprint.h; y++) {
		for (int16 x=0; x<building->footprint.w; x++) {
			city->tileBuildings[tileIndex(city,building->footprint.x+x,building->footprint.y+y)] = buildingID;
		}
	}
	return true;
}

bool demolishTile(City *city, Coord position) {
	if (!tileExists(city, position.x, position.y)) return true;

	uint32 posTI = tileIndex(city, position.x, position.y);

	uint32 buildingID = city->tileBuildings[posTI];
	if (buildingID) {

		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building, "Tile is storing an invalid building ID!");
		BuildingDefinition def = buildingDefinitions[building->archetype];

		// Can we afford to demolish this?
		if (!canAfford(city, def.demolishCost)) {
			pushUiMessage("Not enough money to demolish this.");
			return false;
		}

		spend(city, def.demolishCost);

		// Clear all references to this building
		for (int32 y = building->footprint.y;
			y < building->footprint.y + building->footprint.h;
			y++) {

			for (int32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++) {

				city->tileBuildings[tileIndex(city, x, y)] = 0;
			}
		}
		// Mark the building as non-existent, then next time we create a new building,
		// we'll find the first non-existent building to replace!
		building->exists = false;
		city->buildingCount--;

		// Clear the building's data
		switch (building->archetype) {
			case BA_Farmhouse: {
				city->farmhouse = null;
			} break;

			case BA_Barn: {
				for (uint32 i=0; i<city->barnCount; i++) {
					if (city->barns[i] == building) {
						// Take the last building and move it to position 'i'
						city->barns[i] = city->barns[--city->barnCount];
						break;
					}
				}
			} break;
		}

		return true;

	} else if (city->terrain[posTI] == Terrain_Forest) {
		// Tear down all the trees!
		if (canAfford(city, forestDemolishCost)) {
			spend(city, forestDemolishCost);
			city->terrain[posTI] = Terrain_Ground;
			return true;
		} else {
			pushUiMessage("Not enough money to destroy these trees.");
			return false;
		}

	} else {
		return true;
	}
	
}

int32 calculateDemolitionCost(City *city, Rect rect) {
	int32 total = 0;

	// Terrain clearing cost
	for (int y=0; y<rect.h; y++) {
		for (int x=0; x<rect.w; x++) {
			if (terrainAt(city, rect.x + x, rect.y + y) == Terrain_Forest) {
				total += forestDemolishCost;
			}
		}
	}

	// We want to only get the cost of each building once.
	// So, we'll just iterate through the buildings list. This might be terrible? I dunno.
	for (uint16 i=0; i<city->buildingCountMax; i++) {
		Building building = city->buildings[i];
		if (!building.exists) continue;
		if (rectsOverlap(building.footprint, rect)) {
			total += buildingDefinitions[building.archetype].demolishCost;
		}
	}

	return total;
}

bool demolishRect(City *city, Rect rect) {

	int32 cost = calculateDemolitionCost(city, rect);
	if (!canAfford(city, cost)) {
		pushUiMessage("Not enough money for demolition.");
		return false;
	}

	for (int y=0; y<rect.h; y++) {
		for (int x=0; x<rect.w; x++) {
			if (!demolishTile(city, {rect.x + x, rect.y + y})) {
				return false;
			}
		}
	}

	return true;
}

inline void getCityFundsString(City *city, char *buffer) {
	sprintf(buffer, "£%d", city->funds);
}

inline void updateFundsLabel(City *city, UiLabel *label) {
	char buffer[20];
	getCityFundsString(city, buffer);
	setUiLabelText(label, buffer);
}

void sellAPotato(City *city) {
	city->funds += 100;
}