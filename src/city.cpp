
inline City createCity(uint32 width, uint32 height, char *name, int32 funds) {
	City city = {};
	city.name = name;
	city.funds = funds;
	city.width = width;
	city.height = height;
	city.terrain = new Terrain[width*height]();
	city.tileBuildings = new uint32[width*height]();
	city.buildingCount = 0;
	city.buildingCountMax = ArrayCount(city.buildings);

	city.farmhouse = null;

	return city;
}

inline void freeCity(City *city) {
	delete[] city->terrain;
	delete[] city->tileBuildings;
	*city = {};
}

inline uint32 tileIndex(City *city, uint32 x, uint32 y) {
	return (y * city->width) + x;
}

inline bool tileExists(City *city, uint32 x, uint32 y) {
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Terrain terrainAt(City *city, uint32 x, uint32 y) {
	if (!tileExists(city, x, y)) return Terrain_Invalid;
	return city->terrain[tileIndex(city, x, y)];
}

inline Building* getBuildingByID(City *city, uint32 buildingID) {
	if (buildingID <= 0 || buildingID > city->buildingCountMax) {
		return null;
	}

	return &(city->buildings[buildingID - 1]);
}

inline Building* getBuildingAtPosition(City *city, uint32 x, uint32 y) {
	return getBuildingByID(city, city->tileBuildings[tileIndex(city,x,y)]);
}

void generateTerrain(City *city) {
	for (uint32 y = 0; y < city->height; y++) {
		for (uint32 x = 0; x < city->width; x++) {
			Terrain t = Terrain_Ground;
			if ((rand() % 100) < 5) {
				t = Terrain_Water;
			}
			city->terrain[tileIndex(city,x,y)] = t;
		}
	}
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
	if (city->funds < def.buildCost) {
		if (isAttemptingToBuild) {
			pushUiMessage("Not enough money to build this.");
		}
		return false;
	}

	// Are we in bounds?
	if (!rectInRect({0,0, city->width, city->height}, {position, def.width, def.height})) {
		if (isAttemptingToBuild) {
			pushUiMessage("You cannot build there.");
		}
		return false;
	}

	// Check terrain is buildable and empty
	for (int32 y=0; y<def.height; y++) {
		for (int32 x=0; x<def.width; x++) {
			uint32 ti = tileIndex(city, position.x + x, position.y + y);
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

	city->funds -= def->buildCost;

	*building = {};
	building->exists = true;
	building->archetype = archetype;
	building->footprint = {position, def->width, def->height};

	// Building-specific stuff
	switch (archetype) {
		case BA_Farmhouse: {
			city->farmhouse = building;
		} break;

		case BA_Field: {

			for (int i = 0; i < ArrayCount(city->fieldData); ++i) {
				if (!city->fieldData[i].exists) {
					building->data = city->fieldData + i;
					SDL_Log("Allocating fieldData #%d for building #%d", i, buildingID);
					break;
				}
			}

			ASSERT(building->data != null, "Failed to allocate field data!");

			FieldData *fieldData = (FieldData*)building->data;
			
			fieldData->exists = true;
			fieldData->state = FieldState_Empty;
			fieldData->progress = 0;

		} break;
	}

	for (int16 y=0; y<building->footprint.h; y++) {
		for (int16 x=0; x<building->footprint.w; x++) {
			city->tileBuildings[tileIndex(city,building->footprint.x+x,building->footprint.y+y)] = buildingID;
		}
	}
	return true;
}

bool demolish(City *city, Coord position) {
	if (!tileExists(city, position.x, position.y)) return false;

	uint32 posTI = tileIndex(city, position.x, position.y);

	uint32 buildingID = city->tileBuildings[posTI];
	if (buildingID) {

		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building, "Tile is storing an invalid building ID!");
		BuildingDefinition def = buildingDefinitions[building->archetype];

		// Can we afford to demolish this?
		if (city->funds < def.demolishCost) {
			pushUiMessage("Not enough money to demolish this.");
			return false;
		}

		city->funds -= def.demolishCost;

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

			case BA_Field: {
				FieldData *fieldData = (FieldData*)building->data;
				fieldData->exists = false;
			} break;
		}

		return true;

	} else {
		// TODO: Handle clearing of terrain here.
		pushUiMessage("There is nothing here to demolish.");
		return false;
	}
	
}

inline void getCityFundsString(City *city, char *buffer) {
	sprintf(buffer, "£%d", city->funds);
}

inline void updateFundsLabel(Renderer *renderer, City *city, UiLabel *label) {
	char buffer[20];
	getCityFundsString(city, buffer);
	setUiLabelText(renderer, label, buffer);
}