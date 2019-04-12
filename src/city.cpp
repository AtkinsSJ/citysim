

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;

	s32 tileCount = width*height;

	city->terrain         = PushArray(gameArena, Terrain, tileCount);
	city->tileBuildings   = PushArray(gameArena, s32, tileCount);
	city->pathLayer.data  = PushArray(gameArena, s32, tileCount);
	initialisePowerLayer(gameArena, &city->powerLayer, tileCount);
	initZoneLayer(gameArena, &city->zoneLayer, tileCount);

	city->highestBuildingID = 0;
	initChunkedArray(&city->buildings, gameArena, 1024);
	Building *nullBuilding = appendBlank(&city->buildings);
	nullBuilding->id = 0;
	nullBuilding->typeID = 0;
}

Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint)
{
	s32 buildingArrayIndex = city->buildings.count;
	Building *building = appendBlank(&city->buildings);
	building->id = ++city->highestBuildingID;
	building->typeID = def->typeID;
	building->footprint = footprint;

	for (s32 y=0; y<footprint.h; y++)
	{
		for (s32 x=0; x<footprint.w; x++)
		{
			s32 tile = tileIndex(city, footprint.x+x, footprint.y+y);
			city->tileBuildings[tile] = buildingArrayIndex;
		}
	}

	return building;
}

void generatorPlaceBuilding(City *city, BuildingDef *buildingDef, s32 left, s32 top)
{
	Building *building = addBuilding(city, buildingDef, irectXYWH(left, top, buildingDef->width, buildingDef->height));
	building->textureRegionOffset = randomNext(&globalAppState.cosmeticRandom);
}

void generateTerrain(City *city)
{
	u32 tGround = findTerrainTypeByName(makeString("Ground"));
	u32 tWater  = findTerrainTypeByName(makeString("Water"));

	BuildingDef *bTree = get(&buildingDefs, findBuildingTypeByName(makeString("Tree")));

	for (s32 y = 0; y < city->height; y++) {
		for (s32 x = 0; x < city->width; x++) {

			f32 px = (f32)x * 0.05f;
			f32 py = (f32)y * 0.05f;

			f32 perlinValue = stb_perlin_noise3(px, py, 0);

			Terrain *terrain = &city->terrain[tileIndex(city, x, y)];
			bool isGround = (perlinValue > 0.1f);
			if (isGround)
			{
				terrain->type = tWater;
			}
			else
			{
				terrain->type = tGround;

				if (stb_perlin_noise3(px, py, 1) > 0.2f)
				{
					generatorPlaceBuilding(city, bTree, x, y);
				}
			}

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
			pushUiMessage(uiState, makeString("Not enough money to build this."));
		}
		return false;
	}

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

	// Are we in bounds?
	if (!rectInRect2I(irectXYWH(0,0, city->width, city->height), footprint))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, makeString("You cannot build off the map edge."));
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
					pushUiMessage(uiState, makeString("You cannot build there."));
				}
				return false;
			}

			if (city->tileBuildings[ti] != 0)
			{
				// Check if we can combine this with the building that's already there
				Building *buildingAtPos = get(&city->buildings, city->tileBuildings[ti]);
				if (get(&buildingDefs, buildingAtPos->typeID)->canBeBuiltOnID == selectedBuildingTypeID)
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
	Rect2I footprint = irectXYWH(left, top, def->width, def->height);
	spend(city, def->buildCost);

	bool needToRecalcPaths = def->isPath;
	bool needToRecalcPower = def->carriesPower;

	Building *building = null;
	s32 buildingArrayIndex = city->tileBuildings[tileIndex(city, left, top)];

	if (buildingArrayIndex)
	{
		// Do a quick replace! We already established in canPlaceBuilding() that we match.
		// NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
		building = getBuildingByID(city, buildingArrayIndex);
		ASSERT(building != null, "Somehow this building doesn't exist even though it should!");
		BuildingDef *oldDef = get(&buildingDefs, building->typeID);

		building->typeID = def->buildOverResult;
		def = get(&buildingDefs, def->buildOverResult);

		needToRecalcPaths = (oldDef->isPath != def->isPath);
		needToRecalcPower = (oldDef->carriesPower != def->carriesPower);

		city->totalResidents -= oldDef->residents;
		city->totalJobs -= oldDef->jobs;
	}
	else
	{
		// Remove zones
		placeZone(uiState, city, Zone_None, footprint, false);

		buildingArrayIndex = city->buildings.count;

		building = addBuilding(city, def, footprint);
	}


	// Tiles
	for (s32 y=0; y<footprint.h; y++) {
		for (s32 x=0; x<footprint.w; x++) {
			s32 tile = tileIndex(city,footprint.x+x,footprint.y+y);

			// Data layer updates
			city->pathLayer.data[tile] = def->isPath ? 1 : 0;
		}
	}

	building->currentResidents = 0;
	building->currentJobs = 0;
	city->totalResidents += def->residents;
	city->totalJobs += def->jobs;

	updateBuildingTexture(city, building, def);
	updateAdjacentBuildingTextures(city, footprint);

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
		pushUiMessage(uiState, makeString("Not enough money for construction."));
	}

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			placeBuilding(uiState, city, buildingTypeID, area.x + x, area.y + y, false);
		}
	}
}

// NB: Only for use withing demolishRect()!
bool demolishTile(UIState *uiState, City *city, V2I position)
{
	if (!tileExists(city, position.x, position.y)) return true;

	u32 posTI = tileIndex(city, position.x, position.y);

	s32 buildingID  = city->tileBuildings[posTI];
	Terrain terrain = city->terrain[posTI];

	if (buildingID)
	{
		Building *building = getBuildingByID(city, buildingID);
		ASSERT(building != null, "Tile is storing an invalid building ID!");
		BuildingDef *def = get(&buildingDefs, building->typeID);

		// Can we afford to demolish this?
		if (!canAfford(city, def->demolishCost))
		{
			pushUiMessage(uiState, makeString("Not enough money to demolish this."));
			return false;
		}

		spend(city, def->demolishCost);

		city->totalResidents -= def->residents;
		city->totalJobs -= def->jobs;

		// Clear all references to this building
		for (s32 y = building->footprint.y;
			y < building->footprint.y + building->footprint.h;
			y++)
		{

			for (s32 x = building->footprint.x;
				x < building->footprint.x + building->footprint.w;
				x++)
			{

				s32 tile = tileIndex(city, x, y);

				city->tileBuildings[tile] = 0;

				if (def->isPath)
				{
					// Remove from the pathing layer
					city->pathLayer.data[tile] = 0;
				}
			}
		}

		// Need to update the filled/empty zone lists
		markZonesAsEmpty(city, building->footprint);

		// Update sprites for the building's neighbours.
		updateAdjacentBuildingTextures(city, building->footprint);

		// Overwrite the building record with the highest one
		// Unless it *IS* the highest one!
		// TODO: REPLACE THIS with a system that has consistent IDs!
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
		removeIndex(&city->buildings, city->buildings.count-1, false);

		return true;
	}
	else
	{
		TerrainDef *def = get(&terrainDefs, terrain.type);
		if (def->canDemolish)
		{
			// Tear down all the trees!
			if (canAfford(city, def->demolishCost))
			{
				spend(city, def->demolishCost);
				city->terrain[posTI].type = findTerrainTypeByName(makeString("Ground"));
				return true;
			}
			else
			{
				pushUiMessage(uiState, makeString("Not enough money to clear this terrain."));
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
	for (auto it = iterate(&city->buildings, 1, false); !it.isDone; next(&it))
	{
		Building *building = get(it);

		if (rectsOverlap(building->footprint, area))
		{
			total += get(&buildingDefs, building->typeID)->demolishCost;
		}
	}

	return total;
}

bool demolishRect(UIState *uiState, City *city, Rect2I area) {

	s32 cost = calculateDemolitionCost(city, area);
	if (!canAfford(city, cost)) {
		pushUiMessage(uiState, makeString("Not enough money for demolition."));
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

void calculateDemand(City *city)
{
	// Ratio of residents to job should be roughly 3 : 1

	// TODO: We want to consider AVAILABLE jobs/residents, not TOTAL ones.
	// TODO: This is a generally terrible calculation!

	// Residential
	city->residentialDemand = (city->totalJobs * 3) - city->totalResidents + 50;

	// Commercial
	city->commercialDemand = 0;

	// Industrial
	city->industrialDemand = (city->totalResidents / 3) - city->totalJobs + 30;
}

/**
 * Distance to road, counting diagonal distances as 1.
 * If nothing is found within the maxDistanceToCheck, returns a *really big number*.
 * If the tile itself is a road, just returns 0 as you'd expect.
 */
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck)
{
	s32 result = s32Max;

	if (isPathable(city, x, y))
	{
		result = 0;
	}
	else
	{
		bool done = false;

		for (s32 distance = 1;
			 !done && distance <= maxDistanceToCheck;
			 distance++)
		{
			s32 leftX   = x - distance;
			s32 rightX  = x + distance;
			s32 bottomY = y - distance;
			s32 topY    = y + distance;

			for (s32 px = leftX; px <= rightX; px++)
			{
				if (isPathable(city, px, bottomY))
				{
					result = distance;
					done = true;
					break;
				}

				if (isPathable(city, px, topY))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (isPathable(city, leftX, py))
				{
					result = distance;
					done = true;
					break;
				}

				if (isPathable(city, rightX, py))
				{
					result = distance;
					done = true;
					break;
				}
			}
		}
	}

	return result;
}