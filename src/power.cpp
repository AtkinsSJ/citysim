#pragma once

void initPowerLayer(PowerLayer *layer, City *city, MemoryArena *gameArena)
{
	initChunkedArray(&layer->networks, gameArena, 64);
	initChunkPool(&layer->powerGroupsChunkPool, gameArena, 4);
	initChunkPool(&layer->powerGroupPointersChunkPool, gameArena, 32);

	initSectorGrid(&layer->sectors, gameArena, city->width, city->height, 16);
	for (s32 sectorIndex = 0; sectorIndex < layer->sectors.count; sectorIndex++)
	{
		PowerSector *sector = layer->sectors.sectors + sectorIndex;

		sector->tilePowerGroup = allocateArray<u8>(gameArena, areaOf(sector->bounds));

		initChunkedArray(&sector->powerGroups, &layer->powerGroupsChunkPool);
	}
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

inline u8 getPowerGroupID(PowerSector *sector, s32 x, s32 y)
{
	return *getSectorTile(sector, sector->tilePowerGroup, x, y);
}

inline void setPowerGroupID(PowerSector *sector, s32 x, s32 y, u8 value)
{
	setSectorTile(sector, sector->tilePowerGroup, x, y, value);
}

void updateSectorPowerValues(City *city, PowerSector *sector)
{
	// Reset each to 0
	for (auto it = iterate(&sector->powerGroups);
		!it.isDone;
		next(&it))
	{
		PowerGroup *powerGroup = get(it);
		powerGroup->production = 0;
		powerGroup->consumption = 0;
	}

	// Count power from buildings
	ChunkedArray<Building *> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds, BQF_RequireOriginInArea);
	for (auto it = iterate(&sectorBuildings);
		!it.isDone;
		next(&it))
	{
		Building *building = getValue(it);
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

PowerNetwork *getPowerNetworkAt(City *city, s32 x, s32 y)
{
	PowerNetwork *result = null;

	if (tileExists(city, x, y))
	{
		PowerLayer *powerLayer = &city->powerLayer;
		PowerSector *sector = getSectorAtTilePos(&powerLayer->sectors, x, y);

		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		u8 powerGroupIndex = getPowerGroupID(sector, relX, relY);
		if (powerGroupIndex != 0)
		{
			PowerGroup *group = get(&sector->powerGroups, powerGroupIndex - 1);
			result = get(&powerLayer->networks, group->networkID - 1);
		}
	}

	return result;
}

void floodFillSectorPowerGroup(PowerSector *sector, s32 x, s32 y, u8 fillValue)
{
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

void markPowerLayerDirty(PowerLayer *layer, Rect2I area)
{
	// TODO: Somehow store the area that needs updating, so we don't have to recalculate the whole thing
	
	area = area; // Ignore unused
	layer->isDirty = true;
}

void recalculateSectorPowerGroups(City *city, PowerSector *sector)
{
	DEBUG_FUNCTION();

	// TODO: Clear any references to the PowerGroups that the City itself might have!
	// (I don't know how that's going to be structured yet.)
	// Meaning, if a city-wide power network knows that PowerGroup 3 in this sector is part of it,
	// we need to tell it that PowerGroup 3 is being destroyed!


	// Step 0: Remove the old PowerGroups.
	for (auto it = iterate(&sector->powerGroups); !it.isDone; next(&it))
	{
		PowerGroup *powerGroup = get(it);
		clear(&powerGroup->sectorBoundaries);
	}
	clear(&sector->powerGroups);
	memset(sector->tilePowerGroup, 0, sizeof(sector->tilePowerGroup[0]) * areaOf(sector->bounds));

	// Step 1: Set all power-carrying tiles to -1 (everything was set to 0 in the above memset())

	// - Step 1.1, iterate through our owned buildings and mark their tiles if they carry power
	ChunkedArray<Building *> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds);
	for (auto it = iterate(&sectorBuildings);
		!it.isDone;
		next(&it))
	{
		Building *building = getValue(it);
		BuildingDef *def = getBuildingDef(building->typeID);

		if (def->carriesPower)
		{
			setRectPowerGroupUnknown(sector, building->footprint);
		}
	}

	// - Step 1.2, go through each tile that hasn't already been marked, and check it.
	//   (eg, zones and non-local buildings can carry power)
	for (s32 relY = 0;
		relY < sector->bounds.h;
		relY++)
	{
		for (s32 relX = 0;
			relX < sector->bounds.w;
			relX++)
		{
			// Skip any that have already been set (by step 1.1)
			if (getPowerGroupID(sector, relX, relY) == POWER_GROUP_UNKNOWN) continue;

			ZoneType zone = getZoneAt(city, sector->bounds.x + relX, sector->bounds.y + relY);
			if (zoneDefs[zone].carriesPower)
			{
				setPowerGroupID(sector, relX, relY, POWER_GROUP_UNKNOWN);
			}
			else
			{
				Building *building = getBuildingAtPosition(city, sector->bounds.x + relX, sector->bounds.y + relY);
				if (building != null && getBuildingDef(building->typeID)->carriesPower)
				{
					// Set the building's whole area, so we only do 1 getBuildingAtPosition() lookup per building
					setRectPowerGroupUnknown(sector, building->footprint);
				}
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

			u8 powerGroupID = (u8)sector->powerGroups.count;
			newGroup->production = 0;
			newGroup->consumption = 0;
			floodFillSectorPowerGroup(sector, relX, relY, powerGroupID);
		}
	}

	// At this point, if there are no power groups we can just stop.
	if (sector->powerGroups.count == 0) return;

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
	if (sector->bounds.x + sector->bounds.w < city->width)
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
	if (sector->bounds.y + sector->bounds.h < city->height)
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

void floodFillCityPowerNetwork(PowerLayer *powerLayer, PowerGroup *powerGroup, PowerNetwork *network)
{
	powerGroup->networkID = network->id;
	append(&network->groups, powerGroup);

	for (auto it = iterate(&powerGroup->sectorBoundaries);
		!it.isDone;
		next(&it))
	{
		Rect2I bounds = getValue(it);
		PowerSector *sector = getSectorAtTilePos(&powerLayer->sectors, bounds.x, bounds.y);
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
						floodFillCityPowerNetwork(powerLayer, group, network);
					}
				}
			}
		}
	}
}

/*
 * TODO: Recalculate individual Sectors as needed, instead of recalculating EVERY ONE whenever anything changes!
 * Still need to recalculate the city-wide networks though.
 */
void recalculatePowerConnectivity(City *city, PowerLayer *powerLayer)
{
	DEBUG_FUNCTION();

	// Clean up networks
	for (auto it = iterate(&powerLayer->networks);
		!it.isDone;
		next(&it))
	{
		PowerNetwork *powerNetwork = get(it);
		freePowerNetwork(powerNetwork);
	}
	clear(&powerLayer->networks);

	// Recalculate each sector
	for (s32 sectorIndex = 0;
		sectorIndex < powerLayer->sectors.count;
		sectorIndex++)
	{
		PowerSector *sector = powerLayer->sectors.sectors + sectorIndex;

		recalculateSectorPowerGroups(city, sector);
	}

	// NB: All power groups are on networkID=0 right now, because they all got reconstructed in the above loop.
	// At some point we'll have to manually set that to 0, if we want to recalculate the global networks without
	// recalculating every individual sector.

	// Flood-fill networks of PowerGroups by walking the boundaries
	for (s32 sectorIndex = 0;
		sectorIndex < powerLayer->sectors.count;
		sectorIndex++)
	{
		PowerSector *sector = powerLayer->sectors.sectors + sectorIndex;

		for (auto it = iterate(&sector->powerGroups);
			!it.isDone;
			next(&it))
		{
			PowerGroup *powerGroup = get(it);
			if (powerGroup->networkID == 0)
			{
				PowerNetwork *network = newPowerNetwork(powerLayer);
				floodFillCityPowerNetwork(powerLayer, powerGroup, network);
			}
		}
	}
}

void updatePowerLayer(City *city, PowerLayer *layer)
{
	DEBUG_FUNCTION();

	if (layer->isDirty)
	{
		// TODO: Only update the parts that need updating
		recalculatePowerConnectivity(city, layer);
		layer->isDirty = false;
	}

	layer->cachedCombinedProduction = 0;
	layer->cachedCombinedConsumption = 0;

	// Update each PowerGroup's power
	for (s32 sectorIndex = 0;
		sectorIndex < layer->sectors.count;
		sectorIndex++)
	{
		PowerSector *sector = layer->sectors.sectors + sectorIndex;
		updateSectorPowerValues(city, sector);
	}

	// Sum each PowerGroup's power into its Network
	for (auto networkIt = iterate(&layer->networks);
		!networkIt.isDone;
		next(&networkIt))
	{
		PowerNetwork *network = get(networkIt);
		network->cachedProduction = 0;
		network->cachedConsumption = 0;

		for (auto groupIt = iterate(&network->groups);
			!groupIt.isDone;
			next(&groupIt))
		{
			PowerGroup *powerGroup = getValue(groupIt);
			network->cachedProduction += powerGroup->production;
			network->cachedConsumption += powerGroup->consumption;
		}

		// City-wide power totals
		layer->cachedCombinedProduction  += network->cachedProduction;
		layer->cachedCombinedConsumption += network->cachedConsumption;
	}
}
