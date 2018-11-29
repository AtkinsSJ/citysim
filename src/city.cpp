

void initCity(MemoryArena *gameArena, City *city, u32 width, u32 height, String name, s32 funds)
{
	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;
	city->terrain = PushArray(gameArena, Terrain, width*height);

	city->pathLayer.data = PushArray(gameArena, s32, width*height);


	city->tileBuildings = PushArray(gameArena, u32, width*height);
	city->buildingCount = 1; // For the null building
	city->buildingCountMax = ArrayCount(city->buildings);
}

void generateTerrain(City *city, Random *random)
{
	for (s32 y = 0; y < city->height; y++) {
		for (s32 x = 0; x < city->width; x++) {

			f32 px = (f32)x * 0.1f;
			f32 py = (f32)y * 0.1f;

			f32 perlinValue = stb_perlin_noise3(px, py, 0);

			Terrain *terrain = &city->terrain[tileIndex(city, x, y)];
			terrain->type = (perlinValue > 0.1f)
				? Terrain_Forest
				: Terrain_Ground;
			terrain->textureRegionOffset = (s32) randomNext(&globalAppState.cosmeticRandom);
		}
	}
}

bool canAfford(City *city, s32 cost)
{
	return city->funds >= cost;
}

void spend(City *city, s32 cost)
{
	city->funds -= cost;
}

bool canPlaceBuilding(UIState *uiState, City *city, BuildingArchetype selectedBuildingArchetype, V2I position,
	bool isAttemptingToBuild = false)
{

	// // Only allow one farmhouse!
	// if (selectedBuildingArchetype == BA_Farmhouse
	// 	&& city->firstBuildingOfType[BA_Farmhouse])
	// {
	// 	if (isAttemptingToBuild)
	// 	{
	// 		pushUiMessage(uiState, stringFromChars("You can only have one farmhouse!"));
	// 	}
	// 	return false;
	// }

	BuildingDefinition def = buildingDefinitions[selectedBuildingArchetype];

	// Can we afford to build this?
	if (!canAfford(city, def.buildCost))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, stringFromChars("Not enough money to build this."));
		}
		return false;
	}

	Rect2I footprint = irectCentreWH(position, def.width, def.height);

	// Are we in bounds?
	if (!rectInRect2I(irectXYWH(0,0, city->width, city->height), footprint))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, stringFromChars("You cannot build off the map edge."));
		}
		return false;
	}

	// Check terrain is buildable and empty
	for (s32 y=0; y<def.height; y++)
	{
		for (s32 x=0; x<def.width; x++)
		{
			u32 ti = tileIndex(city, footprint.x + x, footprint.y + y);
			if (city->terrain[ti].type != Terrain_Ground)
			{
				if (isAttemptingToBuild)
				{
					pushUiMessage(uiState, stringFromChars("You cannot build there."));
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

void updatePathTexture(City *city, s32 x, s32 y)
{
	Building *building = getBuildingAtPosition(city, v2i(x, y));
	if (building)
	{
		bool pathU = pathGroupAt(city, x,   y-1) > 0;
		bool pathD = pathGroupAt(city, x,   y+1) > 0;
		bool pathL = pathGroupAt(city, x-1, y  ) > 0;
		bool pathR = pathGroupAt(city, x+1, y  ) > 0;

		building->textureRegionOffset = (pathU ? 1 : 0) | (pathR ? 2 : 0) | (pathD ? 4 : 0) | (pathL ? 8 : 0);
	}
}

/**
 * Attempt to place a building. Returns whether successful.
 */
bool placeBuilding(UIState *uiState, City *city, BuildingArchetype archetype, V2I position)
{

	if (!canPlaceBuilding(uiState, city, archetype, position, true))
	{
		return false;
	}

	ASSERT(city->buildingCount < city->buildingCountMax, "City.buildings is full!");

	u32 buildingID = city->buildingCount++;
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
	for (s16 y=0; y<building->footprint.h; y++) {
		for (s16 x=0; x<building->footprint.w; x++) {
			s32 tile = tileIndex(city,building->footprint.x+x,building->footprint.y+y);
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
		// Sprite id is 0 to 15, depending on connecting neighbours.
		// 1 = up, 2 = right, 4 = down, 8 = left
		// For now, we'll decide that off-the-map does NOT connect
		recalculatePathingConnectivity(city);

		// Update sprites for the tile, and the 4 neighbours.
		updatePathTexture(city, position.x,   position.y);
		updatePathTexture(city, position.x+1, position.y);
		updatePathTexture(city, position.x-1, position.y);
		updatePathTexture(city, position.x,   position.y+1);
		updatePathTexture(city, position.x,   position.y-1);
	}
	else
	{
		// Random sprite!
		building->textureRegionOffset = randomNext(&globalAppState.cosmeticRandom);
	}

	return true;
}

bool demolishTile(UIState *uiState, City *city, V2I position) {
	if (!tileExists(city, position.x, position.y)) return true;

	u32 posTI = tileIndex(city, position.x, position.y);

	u32 buildingID = city->tileBuildings[posTI];
	if (buildingID) {

		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building, "Tile is storing an invalid building ID!");
		BuildingDefinition def = buildingDefinitions[building->archetype];

		// Can we afford to demolish this?
		if (!canAfford(city, def.demolishCost)) {
			pushUiMessage(uiState, stringFromChars("Not enough money to demolish this."));
			return false;
		}

		spend(city, def.demolishCost);

		// Clear all references to this building
		for (s32 y = building->footprint.y;
			y < building->footprint.y + building->footprint.h;
			y++) {

			for (s32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++) {

				s32 tile = tileIndex(city, x, y);

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
			for (s32 y = highest->footprint.y;
				y < highest->footprint.y + highest->footprint.h;
				y++) {

				for (s32 x = highest->footprint.x;
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

	} else if (city->terrain[posTI].type == Terrain_Forest) {
		// Tear down all the trees!
		if (canAfford(city, forestDemolishCost)) {
			spend(city, forestDemolishCost);
			city->terrain[posTI].type = Terrain_Ground;
			return true;
		} else {
			pushUiMessage(uiState, stringFromChars("Not enough money to destroy these trees."));
			return false;
		}

	} else {
		return true;
	}
	
}

s32 calculateDemolitionCost(City *city, Rect2I rect) {
	s32 total = 0;

	// Terrain clearing cost
	for (int y=0; y<rect.h; y++) {
		for (int x=0; x<rect.w; x++) {
			if (terrainAt(city, rect.x + x, rect.y + y)->type == Terrain_Forest) {
				total += forestDemolishCost;
			}
		}
	}

	// We want to only get the cost of each building once.
	// So, we'll just iterate through the buildings list. This might be terrible? I dunno.
	for (u32 i=1; i<=city->buildingCount; i++) {
		Building building = city->buildings[i];
		if (rectsOverlap(building.footprint, rect)) {
			total += buildingDefinitions[building.archetype].demolishCost;
		}
	}

	return total;
}

bool demolishRect(UIState *uiState, City *city, Rect2I rect) {

	s32 cost = calculateDemolitionCost(city, rect);
	if (!canAfford(city, cost)) {
		pushUiMessage(uiState, stringFromChars("Not enough money for demolition."));
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