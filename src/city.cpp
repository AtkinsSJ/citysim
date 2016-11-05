

void initCity(MemoryArena *gameArena, City *city, uint32 width, uint32 height, char *name, int32 funds)
{
	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;
	city->terrain = PushArray(gameArena, Terrain, width*height);

	city->pathLayer.data = PushArray(gameArena, int32, width*height);


	city->tileBuildings = PushArray(gameArena, uint32, width*height);
	city->buildingCount = 1; // For the null building
	city->buildingCountMax = ArrayCount(city->buildings);
}

void generateTerrain(City *city, RandomMT *random)
{

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

bool canAfford(City *city, int32 cost)
{
	return city->funds >= cost;
}

void spend(City *city, int32 cost)
{
	city->funds -= cost;
}

bool canPlaceBuilding(UIState *uiState, City *city, BuildingArchetype selectedBuildingArchetype, Coord position,
	bool isAttemptingToBuild = false)
{

	// Only allow one farmhouse!
	if (selectedBuildingArchetype == BA_Farmhouse
		&& city->firstBuildingOfType[BA_Farmhouse])
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, "You can only have one farmhouse!");
		}
		return false;
	}

	BuildingDefinition def = buildingDefinitions[selectedBuildingArchetype];

	// Can we afford to build this?
	if (!canAfford(city, def.buildCost))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, "Not enough money to build this.");
		}
		return false;
	}

	Rect footprint = irectCentreWH(position, def.width, def.height);

	// Are we in bounds?
	if (!rectInRect(irectXYWH(0,0, city->width, city->height), footprint))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, "You cannot build off the map edge.");
		}
		return false;
	}

	// Check terrain is buildable and empty
	for (int32 y=0; y<def.height; y++)
	{
		for (int32 x=0; x<def.width; x++)
		{
			uint32 ti = tileIndex(city, footprint.x + x, footprint.y + y);
			if (city->terrain[ti] != Terrain_Ground)
			{
				if (isAttemptingToBuild)
				{
					pushUiMessage(uiState, "You cannot build there.");
				}
				return false;
			}

			if (city->tileBuildings[ti] != 0)
			{
				// if (isAttemptingToBuild)
				// {
				// 	pushUiMessage(uiState, "You cannot overlap buildings.");
				// }
				return false;
			}
		}
	}
	return true;
}

/**
 * Attempt to place a building. Returns whether successful.
 */
bool placeBuilding(UIState *uiState, City *city, BuildingArchetype archetype, Coord position) {

	if (!canPlaceBuilding(uiState, city, archetype, position, true)) {
		return false;
	}

	ASSERT(city->buildingCount < city->buildingCountMax, "City.buildings is full!");

	uint32 buildingID = city->buildingCount++;
	Building *building = getBuildingByID(city, buildingID);
	BuildingDefinition *def = buildingDefinitions + archetype;

	spend(city, def->buildCost);

	*building = {};
	building->archetype = archetype;
	building->footprint = irectCentreWH(position, def->width, def->height);

	// Update the building-by-type lists
	Building *firstOfType = city->firstBuildingOfType[archetype];
	if (firstOfType)
	{
		building->nextOfType = firstOfType;
		building->prevOfType = firstOfType->prevOfType;

		building->prevOfType->nextOfType = building->nextOfType->prevOfType = building;
	}
	else
	{
		// We are the only building of this type
		city->firstBuildingOfType[archetype] = building->nextOfType = building->prevOfType = building;
	}

	// Tiles
	for (int16 y=0; y<building->footprint.h; y++) {
		for (int16 x=0; x<building->footprint.w; x++) {
			int32 tile = tileIndex(city,building->footprint.x+x,building->footprint.y+y);
			city->tileBuildings[tile] = buildingID;

			if (def->isPath)
			{
				// Add to the pathing layer
				city->pathLayer.data[tile] = 1;
			}
		}
	}

	if (def->isPath)
	{
		recalculatePathingConnectivity(city);
	}

	return true;
}

bool demolishTile(UIState *uiState, City *city, Coord position) {
	if (!tileExists(city, position.x, position.y)) return true;

	uint32 posTI = tileIndex(city, position.x, position.y);

	uint32 buildingID = city->tileBuildings[posTI];
	if (buildingID) {

		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building, "Tile is storing an invalid building ID!");
		BuildingDefinition def = buildingDefinitions[building->archetype];

		// Can we afford to demolish this?
		if (!canAfford(city, def.demolishCost)) {
			pushUiMessage(uiState, "Not enough money to demolish this.");
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

				int32 tile = tileIndex(city, x, y);

				city->tileBuildings[tile] = 0;

				if (def.isPath)
				{
					// Remove from the pathing layer
					city->pathLayer.data[tile] = 0;
				}
			}
		}

		// Remove the building from its type list
		if (building == building->nextOfType)
		{
			// Only one of type!
			city->firstBuildingOfType[building->archetype] = null;
		}
		else
		{
			if (building == city->firstBuildingOfType[building->archetype])
			{
				city->firstBuildingOfType[building->archetype] = building->nextOfType;
			}

			building->prevOfType->nextOfType = building->nextOfType;
			building->nextOfType->prevOfType = building->prevOfType;
		}

		// Overwrite the building record with the highest one
		// Unless it *IS* the highest one!
		if (buildingID != city->buildingCount)
		{
			Building *highest = getBuildingByID(city, city->buildingCount);

			// Change all references to highest building
			for (int32 y = highest->footprint.y;
				y < highest->footprint.y + highest->footprint.h;
				y++) {

				for (int32 x = highest->footprint.x;
					x < highest->footprint.x + highest->footprint.w;
					x++) {

					city->tileBuildings[tileIndex(city, x, y)] = buildingID;
				}
			}

			// Move it!
			*building = *highest;

			// Update the of-type list
			if (highest == city->firstBuildingOfType[building->archetype])
			{
				city->firstBuildingOfType[building->archetype] = building;
				building->nextOfType = building->prevOfType = building;
			}
			else
			{
				building->prevOfType->nextOfType = building;
				building->nextOfType->prevOfType = building;
			}

			*highest = {};
		}
		city->buildingCount--;

		return true;

	} else if (city->terrain[posTI] == Terrain_Forest) {
		// Tear down all the trees!
		if (canAfford(city, forestDemolishCost)) {
			spend(city, forestDemolishCost);
			city->terrain[posTI] = Terrain_Ground;
			return true;
		} else {
			pushUiMessage(uiState, "Not enough money to destroy these trees.");
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
	for (uint32 i=1; i<=city->buildingCount; i++) {
		Building building = city->buildings[i];
		if (rectsOverlap(building.footprint, rect)) {
			total += buildingDefinitions[building.archetype].demolishCost;
		}
	}

	return total;
}

bool demolishRect(UIState *uiState, City *city, Rect rect) {

	int32 cost = calculateDemolitionCost(city, rect);
	if (!canAfford(city, cost)) {
		pushUiMessage(uiState, "Not enough money for demolition.");
		return false;
	}

	for (int y=0; y<rect.h; y++) {
		for (int x=0; x<rect.w; x++) {
			if (!demolishTile(uiState, city, {rect.x + x, rect.y + y})) {
				return false;
			}
		}
	}

	recalculatePathingConnectivity(city);

	return true;
}