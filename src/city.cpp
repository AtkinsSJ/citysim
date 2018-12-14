

void initCity(MemoryArena *gameArena, City *city, u32 width, u32 height, String name, s32 funds)
{
	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;

	s32 tileCount = width*height;

	city->terrain         = PushArray(gameArena, Terrain, tileCount);
	city->tileBuildings   = PushArray(gameArena, u32, tileCount);
	city->tileZones       = PushArray(gameArena, ZoneType, tileCount);

	city->pathLayer.data  = PushArray(gameArena, s32, tileCount);
	initialisePowerLayer(gameArena, &city->powerLayer, tileCount);

	initialiseArray(&city->buildings, 1024);
	Building *nullBuilding = appendBlank(&city->buildings);
	nullBuilding->typeID = 0;
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

bool canPlaceBuilding(UIState *uiState, City *city, u32 selectedBuildingTypeID, s32 left, s32 top, bool isAttemptingToBuild = false)
{
	BuildingDef *def = get(&buildingDefs, selectedBuildingTypeID);

	// Can we afford to build this?
	if (!canAfford(city, def->buildCost))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, stringFromChars("Not enough money to build this."));
		}
		return false;
	}

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

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
	for (s32 y=0; y<def->height; y++)
	{
		for (s32 x=0; x<def->width; x++)
		{
			u32 ti = tileIndex(city, footprint.x + x, footprint.y + y);

			Terrain terrain = city->terrain[ti];
			TerrainDef *terrainDef = get(&terrainDefs, terrain.type);

			if (!terrainDef->canBuildOn)
			{
				if (isAttemptingToBuild)
				{
					pushUiMessage(uiState, stringFromChars("You cannot build there."));
				}
				return false;
			}

			if (city->tileBuildings[ti] != 0)
			{
				// Check if we can combine this with the building that's already there
				Building buildingAtPos = city->buildings[city->tileBuildings[ti]];
				if (get(&buildingDefs, buildingAtPos.typeID)->canBeBuiltOnID == selectedBuildingTypeID)
				{
					// We can!
				}
				else
				{
					return false;
				}
			}
		}
	}
	return true;
}

void updatePathTexture(City *city, s32 x, s32 y)
{
	// logInfo("updatePathTexture({0}, {1})", {formatInt(x), formatInt(y)});
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
bool placeBuilding(UIState *uiState, City *city, u32 buildingTypeID, s32 left, s32 top, bool showBuildErrors)
{
	if (!canPlaceBuilding(uiState, city, buildingTypeID, left, top, showBuildErrors))
	{
		return false;
	}

	BuildingDef *def = get(&buildingDefs, buildingTypeID);
	spend(city, def->buildCost);

	bool needToRecalcPaths = def->isPath;
	bool needToRecalcPower = def->carriesPower;

	Building *building = null;
	u32 buildingID = city->tileBuildings[tileIndex(city, left, top)];

	if (buildingID)
	{
		// Do a quick replace! We already established in canPlaceBuilding() that we match.
		building = getBuildingByID(city, buildingID);
		ASSERT(building, "Somehow this building doesn't exist even though it should!");
		BuildingDef *oldDef = get(&buildingDefs, building->typeID);

		building->typeID = def->buildOverResult;
		def = get(&buildingDefs, def->buildOverResult);

		needToRecalcPaths = (oldDef->isPath != def->isPath);
		needToRecalcPower = (oldDef->carriesPower != def->carriesPower);
	}
	else
	{
		buildingID = city->buildings.count;
		building = appendBlank(&city->buildings);
		building->typeID = buildingTypeID;
		building->footprint = irectXYWH(left, top, def->width, def->height);
	}

	// Tiles
	for (s32 y=0; y<building->footprint.h; y++) {
		for (s32 x=0; x<building->footprint.w; x++) {
			s32 tile = tileIndex(city,building->footprint.x+x,building->footprint.y+y);
			city->tileBuildings[tile] = buildingID;

			// Data layer updates
			city->pathLayer.data[tile] = def->isPath ? 1 : 0;

			// Remove zones
			ZoneType oldZone = city->tileZones[tile];
			if (oldZone)
			{
				city->tileZones[tile] = Zone_None;

				// We may need to recalculate power!
				bool zoneCarriedPower = zoneDefs[oldZone].carriesPower;
				needToRecalcPower = zoneCarriedPower != def->carriesPower;
			}
		}
	}

	if (def->isPath)
	{
		// Sprite id is 0 to 15, depending on connecting neighbours.
		// 1 = up, 2 = right, 4 = down, 8 = left
		// For now, we'll decide that off-the-map does NOT connect
		ASSERT(def->width == 1 && def->height == 1, "We only support 1x1 path buildings right now!");

		// Update sprites for the tile, and the 4 neighbours.
		updatePathTexture(city, left,   top);
		updatePathTexture(city, left+1, top);
		updatePathTexture(city, left-1, top);
		updatePathTexture(city, left,   top+1);
		updatePathTexture(city, left,   top-1);
	}
	else
	{
		// Random sprite!
		building->textureRegionOffset = randomNext(&globalAppState.cosmeticRandom);
	}

	if (needToRecalcPaths)
	{
		recalculatePathingConnectivity(city);
	}

	if (needToRecalcPower)
	{
		recalculatePowerConnectivity(city);
	}

	return true;
}

s32 calculateBuildCost(City *city, u32 buildingTypeID, Rect2I area)
{
	BuildingDef *def = get(&buildingDefs, buildingTypeID);
	s32 totalCost = 0;

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			if (canPlaceBuilding(null, city, buildingTypeID, area.x + x, area.y + y))
			{
				totalCost += def->buildCost;
			}
		}
	}

	return totalCost;
}

void placeBuildingRect(UIState *uiState, City *city, u32 buildingTypeID, Rect2I area)
{
	BuildingDef *def = get(&buildingDefs, buildingTypeID);

	s32 cost = calculateBuildCost(city, buildingTypeID, area);
	if (!canAfford(city, cost))
	{
		pushUiMessage(uiState, stringFromChars("Not enough money for construction."));
	}

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			placeBuilding(uiState, city, buildingTypeID, area.x + x, area.y + y, false);
		}
	}

	recalculatePathingConnectivity(city);
	recalculatePowerConnectivity(city);
}

// NB: Only for use withing demolishRect()!
bool demolishTile(UIState *uiState, City *city, V2I position) {
	if (!tileExists(city, position.x, position.y)) return true;

	u32 posTI = tileIndex(city, position.x, position.y);

	u32 buildingID  = city->tileBuildings[posTI];
	Terrain terrain = city->terrain[posTI];

	if (buildingID) {

		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building, "Tile is storing an invalid building ID!");
		BuildingDef *def = get(&buildingDefs, building->typeID);

		// Can we afford to demolish this?
		if (!canAfford(city, def->demolishCost)) {
			pushUiMessage(uiState, stringFromChars("Not enough money to demolish this."));
			return false;
		}

		spend(city, def->demolishCost);

		// Clear all references to this building
		for (s32 y = building->footprint.y;
			y < building->footprint.y + building->footprint.h;
			y++) {

			for (s32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++) {

				s32 tile = tileIndex(city, x, y);

				city->tileBuildings[tile] = 0;

				if (def->isPath)
				{
					// Remove from the pathing layer
					city->pathLayer.data[tile] = 0;
				}
			}
		}

		if (def->isPath)
		{
			// Update sprites for the tile's neighbours.
			for (s32 y = building->footprint.y;
				y < building->footprint.y + building->footprint.h;
				y++)
			{
				updatePathTexture(city, building->footprint.x - 1,                     y);
				updatePathTexture(city, building->footprint.x + building->footprint.w, y);
			}

			for (s32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++)
			{
				updatePathTexture(city, x, building->footprint.y - 1);
				updatePathTexture(city, x, building->footprint.y + building->footprint.h);
			}
		}

		// Overwrite the building record with the highest one
		// Unless it *IS* the highest one!
		if (buildingID != (city->buildings.count-1))
		{
			Building *highest = getBuildingByID(city, city->buildings.count - 1);

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

			*highest = {};
		}
		city->buildings.count--;

		return true;
	}
	else
	{
		TerrainDef *def = get(&terrainDefs, terrain.type);
		if (def->canDemolish)
		{
			// Tear down all the trees!
			if (canAfford(city, def->demolishCost)) {
				spend(city, def->demolishCost);
				city->terrain[posTI].type = Terrain_Ground;
				return true;
			} else {
				pushUiMessage(uiState, stringFromChars("Not enough money to clear this terrain."));
				return false;
			}
		}
		else
		{
			return true;
		}
	}
}

s32 calculateDemolitionCost(City *city, Rect2I area)
{
	s32 total = 0;

	// Terrain clearing cost
	for (int y=0; y<area.h; y++)
	{
		for (int x=0; x<area.w; x++)
		{
			TerrainDef *tDef = get(&terrainDefs, terrainAt(city, area.x + x, area.y + y).type);

			if (tDef->canDemolish)
			{
				total += tDef->demolishCost;
			}
		}
	}

	// We want to only get the cost of each building once.
	// So, we'll just iterate through the buildings list. This might be terrible? I dunno.
	// TODO: Make this instead do a position-based query, keeping track of checked buildings
	for (u32 i=1; i<city->buildings.count; i++)
	{
		Building building = city->buildings[i];
		if (rectsOverlap(building.footprint, area))
		{
			total += get(&buildingDefs, building.typeID)->demolishCost;
		}
	}

	return total;
}

bool demolishRect(UIState *uiState, City *city, Rect2I area) {

	s32 cost = calculateDemolitionCost(city, area);
	if (!canAfford(city, cost)) {
		pushUiMessage(uiState, stringFromChars("Not enough money for demolition."));
		return false;
	}

	for (int y=0; y<area.h; y++) {
		for (int x=0; x<area.w; x++) {
			if (!demolishTile(uiState, city, {area.x + x, area.y + y})) {
				return false;
			}
		}
	}

	recalculatePathingConnectivity(city);
	recalculatePowerConnectivity(city);

	return true;
}
