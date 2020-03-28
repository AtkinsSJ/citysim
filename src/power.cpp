#pragma once

void initPowerLayer(PowerLayer *layer, City *city, MemoryArena *gameArena)
{
	initChunkedArray(&layer->networks, gameArena, 64);
	initChunkPool(&layer->powerGroupsChunkPool, gameArena, 4);
	initChunkPool(&layer->powerGroupPointersChunkPool, gameArena, 32);

	s32 cityArea = areaOf(city->bounds);
	layer->tilePowerDistance = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tilePowerDistance, 255, cityArea);
	layer->powerMaxDistance = 2;
	initDirtyRects(&layer->dirtyRects, gameArena, layer->powerMaxDistance, city->bounds);

	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16);
	for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++)
	{
		PowerSector *sector = &layer->sectors.sectors[sectorIndex];

		sector->tilePowerGroup = allocateMultiple<u8>(gameArena, areaOf(sector->bounds));

		initChunkedArray(&sector->powerGroups, &layer->powerGroupsChunkPool);
	}

	initChunkedArray(&layer->powerBuildings, &city->buildingRefsChunkPool);
}

PowerNetwork *newPowerNetwork(PowerLayer *layer)
{
	PowerNetwork *network = appendBlank(&layer->networks);
	network->id = (s32) layer->networks.count;
	initChunkedArray(&network->groups, &layer->powerGroupPointersChunkPool);

	return network;
}

void freePowerNetwork(PowerNetwork *network)
{
	network->id = 0;
	clear(&network->groups);
}

inline u8 getPowerGroupID(PowerSector *sector, s32 relX, s32 relY)
{
	return *getSectorTile(sector, sector->tilePowerGroup, relX, relY);
}

inline void setPowerGroupID(PowerSector *sector, s32 relX, s32 relY, u8 value)
{
	setSectorTile(sector, sector->tilePowerGroup, relX, relY, value);
}

PowerGroup *getPowerGroupAt(PowerSector *sector, s32 relX, s32 relY)
{
	PowerGroup *result = null;

	if (sector != null)
	{
		s32 powerGroupID = getPowerGroupID(sector, relX, relY);
		if (powerGroupID != 0 && powerGroupID != POWER_GROUP_UNKNOWN)
		{
			result = get(&sector->powerGroups, powerGroupID - 1);
		}
	}

	return result;
}

u8 getDistanceToPower(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->powerLayer.tilePowerDistance, x, y);
}

void drawPowerDataView(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	PowerLayer *layer = &city->powerLayer;

	Array<V4> *palette = getPalette("power"_s);
	u8 paletteIndexPowered  = 0;
	u8 paletteIndexBrownout = 1;
	u8 paletteIndexBlackout = 2;
	u8 paletteIndexNone     = 255;

	Array2<u8> grid = allocateArray2<u8>(tempArena, visibleTileBounds.w, visibleTileBounds.h);
	for (s32 gridY = 0;
		gridY < grid.h;
		gridY++)
	{
		for (s32 gridX = 0;
			gridX < grid.w;
			gridX++)
		{
			PowerNetwork *network = getPowerNetworkAt(city, visibleTileBounds.x + gridX, visibleTileBounds.y + gridY);
			if (network == null)
			{
				grid.set(gridX, gridY, paletteIndexNone);
			}
			else
			{
				if (network->cachedProduction == 0)
				{
					grid.set(gridX, gridY, paletteIndexBlackout);
				}
				else if (network->cachedProduction < network->cachedConsumption)
				{
					grid.set(gridX, gridY, paletteIndexBrownout);
				}
				else
				{
					grid.set(gridX, gridY, paletteIndexPowered);
				}
			}
		}
	}

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), grid.w, grid.h, grid.items, (u16)palette->count, palette->items);

	// Highlight power stations
	drawBuildingHighlights(city, &layer->powerBuildings);
}

void updateSectorPowerValues(City *city, PowerSector *sector)
{
	DEBUG_FUNCTION();

	// Reset each to 0
	for (auto it = iterate(&sector->powerGroups);
		hasNext(&it);
		next(&it))
	{
		PowerGroup *powerGroup = get(&it);
		powerGroup->production = 0;
		powerGroup->consumption = 0;
	}

	// Count power from buildings
	ChunkedArray<Building *> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds, BQF_RequireOriginInArea);
	for (auto it = iterate(&sectorBuildings);
		hasNext(&it);
		next(&it))
	{
		Building *building = getValue(&it);
		BuildingDef *def = getBuildingDef(building->typeID);

		if (def->power != 0)
		{
			u8 powerGroupIndex = getPowerGroupID(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);
			PowerGroup *powerGroup = get(&sector->powerGroups, powerGroupIndex-1);
			if (def->power > 0)
			{
				powerGroup->production += def->power;
			}
			else
			{
				powerGroup->consumption -= def->power;
			}
		}
	}
}

bool doesTileHavePowerNetwork(City *city, s32 x, s32 y)
{
	bool result = false;

	if (tileExists(city, x, y))
	{
		PowerLayer *layer = &city->powerLayer;
		PowerSector *sector = getSectorAtTilePos(&layer->sectors, x, y);

		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		u8 powerGroupIndex = getPowerGroupID(sector, relX, relY);
		result = (powerGroupIndex != 0);
	}

	return result;
}

PowerNetwork *getPowerNetworkAt(City *city, s32 x, s32 y)
{
	PowerNetwork *result = null;

	if (tileExists(city, x, y))
	{
		PowerLayer *layer = &city->powerLayer;
		PowerSector *sector = getSectorAtTilePos(&layer->sectors, x, y);

		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		u8 powerGroupIndex = getPowerGroupID(sector, relX, relY);
		if (powerGroupIndex != 0)
		{
			PowerGroup *group = get(&sector->powerGroups, powerGroupIndex - 1);
			result = get(&layer->networks, group->networkID - 1);
		}
	}

	return result;
}

void floodFillSectorPowerGroup(PowerSector *sector, s32 x, s32 y, u8 fillValue)
{
	DEBUG_FUNCTION();

	// Theoretically, the only place we non-recursively call this is in recalculateSectorPowerGroups(),
	// where we go top-left to bottom-right, so we only need to flood fill right and down from the
	// initial point!
	// However, I'm not sure right now if we might want to flood fill starting from some other point,
	// so I'll leave this as is.
	// Actually... no, that's not correct, because we could have this situation:
	//
	// .................
	// .............@...
	// ...######....#...
	// ...#....######...
	// ...#.............
	// .................
	//
	// Starting at @, following the path of power lines (#) takes us left and up!
	// So, yeah. It's fine as is!
	//
	// - Sam, 10/06/2019
	//

	setPowerGroupID(sector, x, y, fillValue);

	if ((x > 0) && (getPowerGroupID(sector, x-1, y) == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x-1, y, fillValue);
	}

	if ((x < sector->bounds.w-1) && (getPowerGroupID(sector, x+1, y) == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x+1, y, fillValue);
	}

	if ((y > 0) && (getPowerGroupID(sector, x, y-1) == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y-1, fillValue);
	}
	
	if ((y < sector->bounds.h-1) && (getPowerGroupID(sector, x, y+1) == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y+1, fillValue);
	}
}

inline void setRectPowerGroupUnknown(PowerSector *sector, Rect2I area)
{
	DEBUG_FUNCTION();

	Rect2I relArea = intersectRelative(sector->bounds, area);

	for (s32 relY=relArea.y;
		relY < relArea.y + relArea.h;
		relY++)
	{
		for (s32 relX=relArea.x;
			relX < relArea.x + relArea.w;
			relX++)
		{
			setPowerGroupID(sector, relX, relY, POWER_GROUP_UNKNOWN);
		}
	}
}

void markPowerLayerDirty(PowerLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void addBuildingToPowerLayer(PowerLayer *layer, Building *building)
{
	PowerSector *sector = getSectorAtTilePos(&layer->sectors, building->footprint.x, building->footprint.y);
	PowerGroup *group = getPowerGroupAt(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);
	if (group != null)
	{
		append(&group->buildings, getReferenceTo(building));
	}
}

void recalculateSectorPowerGroups(City *city, PowerSector *sector)
{
	DEBUG_FUNCTION();

	PowerLayer *layer = &city->powerLayer;

	// TODO: Clear any references to the PowerGroups that the City itself might have!
	// (I don't know how that's going to be structured yet.)
	// Meaning, if a city-wide power network knows that PowerGroup 3 in this sector is part of it,
	// we need to tell it that PowerGroup 3 is being destroyed!


	// Step 0: Remove the old PowerGroups.
	for (auto it = iterate(&sector->powerGroups); hasNext(&it); next(&it))
	{
		PowerGroup *powerGroup = get(&it);
		clear(&powerGroup->sectorBoundaries);
	}
	clear(&sector->powerGroups);
	fillMemory<u8>(sector->tilePowerGroup, 0, sizeof(sector->tilePowerGroup[0]) * areaOf(sector->bounds));

	// Step 1: Set all power-carrying tiles to POWER_GROUP_UNKNOWN (everything was set to 0 in the above memset())
	for (s32 relY = 0;
		relY < sector->bounds.h;
		relY++)
	{
		for (s32 relX = 0;
			relX < sector->bounds.w;
			relX++)
		{
			u8 distanceToPower = getTileValue(city, layer->tilePowerDistance, relX + sector->bounds.x, relY + sector->bounds.y);

			if (distanceToPower <= 1)
			{
				setPowerGroupID(sector, relX, relY, POWER_GROUP_UNKNOWN);
			}
			else
			{
				setPowerGroupID(sector, relX, relY, 0);
			}
		}
	}

	// Step 2: Flood fill each -1 tile as a local PowerGroup
	for (s32 relY = 0;
		relY < sector->bounds.h;
		relY++)
	{
		for (s32 relX = 0;
			relX < sector->bounds.w;
			relX++)
		{
			// Skip tiles that have already been added to a PowerGroup
			if (getPowerGroupID(sector, relX, relY) != POWER_GROUP_UNKNOWN) continue;

			PowerGroup *newGroup = appendBlank(&sector->powerGroups);

			initChunkedArray(&newGroup->sectorBoundaries, &city->sectorBoundariesChunkPool);
			initChunkedArray(&newGroup->buildings, &city->buildingRefsChunkPool);

			u8 powerGroupID = (u8)sector->powerGroups.count;
			newGroup->production = 0;
			newGroup->consumption = 0;
			floodFillSectorPowerGroup(sector, relX, relY, powerGroupID);
		}
	}

	// At this point, if there are no power groups we can just stop.
	if (sector->powerGroups.count == 0) return;

	// Store references to the buildings in each group, for faster updating later
	ChunkedArray<Building *> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds);
	for (auto it = iterate(&sectorBuildings);
		hasNext(&it);
		next(&it))
	{
		Building *building = getValue(&it);
		if (getBuildingDef(building->typeID)->power == 0) continue; // We only care about powered buildings!

		if (contains(sector->bounds, building->footprint.pos))
		{
			PowerGroup *group = getPowerGroupAt(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);

			ASSERT(group != null);
			append(&group->buildings, getReferenceTo(building));
		}
	}


	// Step 3: Calculate power production/consumption for OWNED buildings, and add to their PowerGroups
	updateSectorPowerValues(city, sector);

	// Step 4: Find and store the PowerGroup boundaries along the sector's edges, on the OUTSIDE
	// @Copypasta The code for all this is really repetitive, but I'm not sure how to factor it together nicely.

	// - Step 4.1: Left edge
	if (sector->bounds.x > 0)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relX = 0;
		for (s32 relY = 0;
			relY < sector->bounds.h;
			relY++)
		{
			u8 tilePGId = getPowerGroupID(sector, relX, relY);

			if (tilePGId == 0)
			{
				currentPGId = 0;
			}
			else if (tilePGId == currentPGId)
			{
				// Extend it
				currentBoundary->h++;
			}
			else
			{
				currentPGId = tilePGId;

				// Start a new boundary
				currentBoundary = appendBlank(&get(&sector->powerGroups, currentPGId-1)->sectorBoundaries);
				currentBoundary->x = sector->bounds.x - 1;
				currentBoundary->y = sector->bounds.y + relY;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.2: Right edge
	if (sector->bounds.x + sector->bounds.w < city->bounds.w)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relX = sector->bounds.w-1;
		for (s32 relY = 0;
			relY < sector->bounds.h;
			relY++)
		{
			u8 tilePGId = getPowerGroupID(sector, relX, relY);

			if (tilePGId == 0)
			{
				currentPGId = 0;
			}
			else if (tilePGId == currentPGId)
			{
				// Extend it
				currentBoundary->h++;
			}
			else
			{
				currentPGId = tilePGId;

				// Start a new boundary
				currentBoundary = appendBlank(&get(&sector->powerGroups, currentPGId-1)->sectorBoundaries);
				currentBoundary->x = sector->bounds.x + sector->bounds.w;
				currentBoundary->y = sector->bounds.y + relY;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.3: Top edge
	if (sector->bounds.y > 0)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relY = 0;
		for (s32 relX = 0;
			relX < sector->bounds.w;
			relX++)
		{
			u8 tilePGId = getPowerGroupID(sector, relX, relY);

			if (tilePGId == 0)
			{
				currentPGId = 0;
			}
			else if (tilePGId == currentPGId)
			{
				// Extend it
				currentBoundary->w++;
			}
			else
			{
				currentPGId = tilePGId;

				// Start a new boundary
				currentBoundary = appendBlank(&get(&sector->powerGroups, currentPGId-1)->sectorBoundaries);
				currentBoundary->x = sector->bounds.x + relX;
				currentBoundary->y = sector->bounds.y - 1;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.4: Bottom edge
	if (sector->bounds.y + sector->bounds.h < city->bounds.h)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relY = sector->bounds.h-1;
		for (s32 relX = 0;
			relX < sector->bounds.w;
			relX++)
		{
			u8 tilePGId = getPowerGroupID(sector, relX, relY);

			if (tilePGId == 0)
			{
				currentPGId = 0;
			}
			else if (tilePGId == currentPGId)
			{
				// Extend it
				currentBoundary->w++;
			}
			else
			{
				currentPGId = tilePGId;

				// Start a new boundary
				currentBoundary = appendBlank(&get(&sector->powerGroups, currentPGId-1)->sectorBoundaries);
				currentBoundary->x = sector->bounds.x + relX;
				currentBoundary->y = sector->bounds.y + sector->bounds.h;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	//
	// Step 5? Could now store a list of direct pointers or indices to connected power groups in other sectors.
	// We'd need to then clean those references up in step 0, but it could make things faster - no need to
	// query for the (0 to many) power groups covered by the boundaries whenever we need to walk the graph.
	// However, we probably don't need to walk it very often! Right now, I can only think of the "assign each
	// power group to a global network" procedure as needing to do this. (I guess when calculating which sectors
	// get power if the power prod < consumption, that also requires a walk? But maybe not. I really don't know
	// how I want that to work.)
	//
	// So, yeah! Could be useful or could not, but THIS is the place to do that work. (Or maybe directly in step 4?)
	// Though, we'd need to make sure we add the references on both sides, because otherwise things could go out
	// of sync, because we'd be building those links before some sectors have been calculated. UGH. Maybe it IS
	// better to just do that walk after we're all done!
	//
	// Sam, 12/06/2019
	//
}

void floodFillCityPowerNetwork(PowerLayer *layer, PowerGroup *powerGroup, PowerNetwork *network)
{
	DEBUG_FUNCTION();

	powerGroup->networkID = network->id;
	append(&network->groups, powerGroup);

	for (auto it = iterate(&powerGroup->sectorBoundaries);
		hasNext(&it);
		next(&it))
	{
		Rect2I bounds = getValue(&it);
		PowerSector *sector = getSectorAtTilePos(&layer->sectors, bounds.x, bounds.y);
		bounds = intersectRelative(sector->bounds, bounds);

		s32 lastPowerGroupIndex = -1;

		// TODO: @Speed We could probably just do 1 loop because the bounds rect is only 1-wide in one dimension!
		for (s32 relY = bounds.y; relY < bounds.y + bounds.h; relY++)
		{
			for (s32 relX = bounds.x; relX < bounds.x + bounds.w; relX++)
			{
				s32 powerGroupIndex = getPowerGroupID(sector, relX, relY);
				if (powerGroupIndex != 0 && powerGroupIndex != lastPowerGroupIndex)
				{
					lastPowerGroupIndex = powerGroupIndex;
					PowerGroup *group = get(&sector->powerGroups, powerGroupIndex - 1);
					if (group->networkID != network->id)
					{
						floodFillCityPowerNetwork(layer, group, network);
					}
				}
			}
		}
	}
}

void recalculatePowerConnectivity(PowerLayer *layer)
{
	DEBUG_FUNCTION();

	// Clean up networks
	for (auto networkIt = iterate(&layer->networks);
		hasNext(&networkIt);
		next(&networkIt))
	{
		PowerNetwork *powerNetwork = get(&networkIt);

		for (auto groupIt = iterate(&powerNetwork->groups);
			hasNext(&groupIt);
			next(&groupIt))
		{
			PowerGroup *group = getValue(&groupIt);
			group->networkID = 0;
		}

		freePowerNetwork(powerNetwork);
	}
	clear(&layer->networks);

	// NB: All power groups are on networkID=0 right now, because they all got reconstructed in the above loop.
	// At some point we'll have to manually set that to 0, if we want to recalculate the global networks without
	// recalculating every individual sector.

	// Flood-fill networks of PowerGroups by walking the boundaries
	for (s32 sectorIndex = 0;
		sectorIndex < getSectorCount(&layer->sectors);
		sectorIndex++)
	{
		PowerSector *sector = &layer->sectors.sectors[sectorIndex];

		for (auto it = iterate(&sector->powerGroups);
			hasNext(&it);
			next(&it))
		{
			PowerGroup *powerGroup = get(&it);
			if (powerGroup->networkID == 0)
			{
				PowerNetwork *network = newPowerNetwork(layer);
				floodFillCityPowerNetwork(layer, powerGroup, network);
			}
		}
	}
}

void updatePowerLayer(City *city, PowerLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		Set<PowerSector *> touchedSectors;
		initSet<PowerSector *>(&touchedSectors, tempArena, [](PowerSector **a, PowerSector **b) { return *a == *b; });

		for (auto it = iterate(&layer->dirtyRects.rects);
			hasNext(&it);
			next(&it))
		{
			Rect2I dirtyRect = getValue(&it);

			// Clear the "distance to power" for the surrounding area to 0 or 255
			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					Building *building = getBuildingAt(city, x, y);
					BuildingDef *def = null;
					if (building != null)
					{
						def = getBuildingDef(building->typeID);
					}

					if (def != null && def->flags & Building_CarriesPower)
					{
						setTile<u8>(city, layer->tilePowerDistance, x, y, 0);
					}
					else if (getZoneDef(getZoneAt(city, x, y)).carriesPower)
					{
						setTile<u8>(city, layer->tilePowerDistance, x, y, 0);
					}
					else
					{
						setTile<u8>(city, layer->tilePowerDistance, x, y, 255);
					}
				}
			}

			// Add the sectors to the list of touched sectors
			Rect2I sectorsRect = getSectorsCovered(&layer->sectors, dirtyRect);
			for (s32 sY = sectorsRect.y; sY < sectorsRect.y + sectorsRect.h; sY++)
			{
				for (s32 sX = sectorsRect.x; sX < sectorsRect.x + sectorsRect.w; sX++)
				{
					add(&touchedSectors, getSector(&layer->sectors, sX, sY));
				}
			}
		}

		// Recalculate distance
		updateDistances(city, layer->tilePowerDistance, &layer->dirtyRects, layer->powerMaxDistance);

		// Rebuild the sectors that were modified
		for (auto it = iterate(&touchedSectors); hasNext(&it); next(&it))
		{
			PowerSector *sector = getValue(&it);

			recalculateSectorPowerGroups(city, sector);
		}

		recalculatePowerConnectivity(layer);
		clearDirtyRects(&layer->dirtyRects);
	}

	layer->cachedCombinedProduction = 0;
	layer->cachedCombinedConsumption = 0;

	// Update each PowerGroup's power
	for (s32 sectorIndex = 0;
		sectorIndex < getSectorCount(&layer->sectors);
		sectorIndex++)
	{
		PowerSector *sector = &layer->sectors.sectors[sectorIndex];
		updateSectorPowerValues(city, sector);
	}

	// Sum each PowerGroup's power into its Network
	for (auto networkIt = iterate(&layer->networks);
		hasNext(&networkIt);
		next(&networkIt))
	{
		PowerNetwork *network = get(&networkIt);
		network->cachedProduction = 0;
		network->cachedConsumption = 0;

		for (auto groupIt = iterate(&network->groups);
			hasNext(&groupIt);
			next(&groupIt))
		{
			PowerGroup *powerGroup = getValue(&groupIt);
			network->cachedProduction += powerGroup->production;
			network->cachedConsumption += powerGroup->consumption;
		}

		// City-wide power totals
		layer->cachedCombinedProduction  += network->cachedProduction;
		layer->cachedCombinedConsumption += network->cachedConsumption;
	}

	// Handle blackout/brownout situations
	for (auto networkIt = iterate(&layer->networks);
		hasNext(&networkIt);
		next(&networkIt))
	{
		PowerNetwork *network = get(&networkIt);

		if (network->cachedConsumption == 0)
		{
			// No consumers, so it's fine
			continue;
		}
		else if (network->cachedProduction == 0)
		{
			// Blackout!
			// @Copypasta Almost identical code for blackout/fully-power

			// So, mark every consumer as having no power
			for (auto groupIt = iterate(&network->groups);
				hasNext(&groupIt);
				next(&groupIt))
			{
				PowerGroup *powerGroup = getValue(&groupIt);

				for (auto buildingRefIt = iterate(&powerGroup->buildings);
					hasNext(&buildingRefIt);
					next(&buildingRefIt))
				{
					BuildingRef buildingRef = getValue(&buildingRefIt);
					Building *building = getBuilding(city, buildingRef);

					if (building != null)
					{
						addProblem(building, BuildingProblem_NoPower);
					}
				}
			}
		}
		else if (network->cachedProduction < network->cachedConsumption)
		{
			// Brownout!
			// Supply power to buildings if we can, and mark the rest as unpowered.
			// TODO: Implement some kind of "rolling brownout" system where buildings get powered
			// and unpowered over time to even things out, instead of it always being first-come-first-served.
			s32 powerRemaining = network->cachedProduction;

			// @Copypasta Only a little different from the other branches
			for (auto groupIt = iterate(&network->groups);
				hasNext(&groupIt);
				next(&groupIt))
			{
				PowerGroup *powerGroup = getValue(&groupIt);

				for (auto buildingRefIt = iterate(&powerGroup->buildings);
					hasNext(&buildingRefIt);
					next(&buildingRefIt))
				{
					BuildingRef buildingRef = getValue(&buildingRefIt);
					Building *building = getBuilding(city, buildingRef);

					if (building != null)
					{
						s32 requiredPower = getRequiredPower(building);
						if (powerRemaining >= requiredPower)
						{
							removeProblem(building, BuildingProblem_NoPower);
							powerRemaining -= requiredPower;
						}
						else
						{
							addProblem(building, BuildingProblem_NoPower);
						}
					}
				}
			}
		}
		else
		{
			// All buildings have power!
			// @Copypasta Almost identical code for blackout/fully-power

			// So, mark every consumer as having power
			for (auto groupIt = iterate(&network->groups);
				hasNext(&groupIt);
				next(&groupIt))
			{
				PowerGroup *powerGroup = getValue(&groupIt);

				for (auto buildingRefIt = iterate(&powerGroup->buildings);
					hasNext(&buildingRefIt);
					next(&buildingRefIt))
				{
					BuildingRef buildingRef = getValue(&buildingRefIt);
					Building *building = getBuilding(city, buildingRef);

					if (building != null)
					{
						removeProblem(building, BuildingProblem_NoPower);
					}
				}
			}
		}
	}
}

void registerPowerBuilding(PowerLayer *layer, Building *building)
{
	append(&layer->powerBuildings, getReferenceTo(building));
}

void unregisterPowerBuilding(PowerLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->powerBuildings, getReferenceTo(building));
	ASSERT(success);
}

void debugInspectPower(WindowContext *context, City *city, s32 x, s32 y)
{
	window_label(context, "*** POWER INFO ***"_s);

	// Power group
	PowerNetwork *powerNetwork = getPowerNetworkAt(city, x, y);
	if (powerNetwork != null)
	{
		window_label(context, myprintf("Power Network {0}:\n- Production: {1}\n- Consumption: {2}\n- Contained groups: {3}"_s, {
			formatInt(powerNetwork->id),
			formatInt(powerNetwork->cachedProduction),
			formatInt(powerNetwork->cachedConsumption),
			formatInt(powerNetwork->groups.count)
		}));
	}

	window_label(context, myprintf("Distance to power: {0}"_s, {formatInt(getDistanceToPower(city, x, y))}));
}
