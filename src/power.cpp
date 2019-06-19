#pragma once

void initPowerLayer(PowerLayer *layer, City *city, MemoryArena *gameArena)
{
	initChunkedArray(&layer->networks, gameArena, 64);
	initChunkPool(&layer->powerGroupsChunkPool, gameArena, 4);
	initChunkPool(&layer->powerGroupPointersChunkPool, gameArena, 32);

	layer->sectorsX = divideCeil(city->width,  SECTOR_SIZE);
	layer->sectorsY = divideCeil(city->height, SECTOR_SIZE);
	layer->sectorCount = layer->sectorsX * layer->sectorsY;
	layer->sectors = PushArray(gameArena, PowerSector, layer->sectorCount);

	for (s32 y = 0; y < layer->sectorsY; y++)
	{
		for (s32 x = 0; x < layer->sectorsX; x++)
		{
			PowerSector *sector = layer->sectors + (layer->sectorsX * y) + x;

			*sector = {};
			sector->base = getSector(city, x, y);

			initChunkedArray(&sector->powerGroups, &layer->powerGroupsChunkPool);
		}
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

void updateSectorPowerValues(PowerSector *sector)
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
	for (auto it = iterate(&sector->base->buildings);
		!it.isDone;
		next(&it))
	{
		Building *building = get(it);
		BuildingDef *def = getBuildingDef(building->typeID);

		if (def->power != 0)
		{
			u8 powerGroupIndex = sector->tilePowerGroup[building->footprint.y - sector->base->bounds.y][building->footprint.x - sector->base->bounds.x];
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
		PowerSector *sector = powerLayer->sectors + getSectorIndexAtTilePos(x, y, city->sectorsX);

		s32 relX = x - sector->base->bounds.x;
		s32 relY = y - sector->base->bounds.y;

		s32 powerGroupIndex = sector->tilePowerGroup[relY][relX];
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

	sector->tilePowerGroup[y][x] = fillValue;

	if ((x > 0) && (sector->tilePowerGroup[y][x-1] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x-1, y, fillValue);
	}

	if ((x < sector->base->bounds.w-1) && (sector->tilePowerGroup[y][x+1] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x+1, y, fillValue);
	}

	if ((y > 0) && (sector->tilePowerGroup[y-1][x] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y-1, fillValue);
	}
	
	if ((y < sector->base->bounds.h-1) && (sector->tilePowerGroup[y+1][x] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y+1, fillValue);
	}
}

inline void setRectPowerGroupUnknown(PowerSector *sector, Rect2I area)
{
	Rect2I relArea = intersectRelative(area, sector->base->bounds);

	for (s32 relY=relArea.y;
		relY < relArea.y + relArea.h;
		relY++)
	{
		for (s32 relX=relArea.x;
			relX < relArea.x + relArea.w;
			relX++)
		{
			sector->tilePowerGroup[relY][relX] = POWER_GROUP_UNKNOWN;
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
	memset(&sector->tilePowerGroup, 0, sizeof(sector->tilePowerGroup));

	// Step 1: Set all power-carrying tiles to -1 (everything was set to 0 in the above memset())

	// - Step 1.1, iterate through our owned buildings and mark their tiles if they carry power
	for (auto it = iterate(&sector->base->buildings); !it.isDone; next(&it))
	{
		Building *building = get(it);
		BuildingDef *def = getBuildingDef(building->typeID);

		if (def->carriesPower)
		{
			setRectPowerGroupUnknown(sector, building->footprint);
		}
	}

	// - Step 1.2, go through each tile that hasn't already been marked, and check it.
	//   (eg, zones and non-local buildings can carry power)
	for (s32 relY = 0;
		relY < sector->base->bounds.h;
		relY++)
	{
		for (s32 relX = 0;
			relX < sector->base->bounds.w;
			relX++)
		{
			// Skip any that have already been set (by step 1.1)
			if (sector->tilePowerGroup[relY][relX] == POWER_GROUP_UNKNOWN) continue;

			ZoneType zone = sector->base->tileZone[relY][relX];
			if (zoneDefs[zone].carriesPower)
			{
				sector->tilePowerGroup[relY][relX] = POWER_GROUP_UNKNOWN;
			}
			else
			{
				Building *building = getBuildingAtPosition(city, sector->base->bounds.x + relX, sector->base->bounds.y + relY);
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
		relY < sector->base->bounds.h;
		relY++)
	{
		for (s32 relX = 0;
			relX < sector->base->bounds.w;
			relX++)
		{
			// Skip tiles that have already been added to a PowerGroup
			if (sector->tilePowerGroup[relY][relX] != POWER_GROUP_UNKNOWN) continue;

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
	updateSectorPowerValues(sector);

	// Step 4: Find and store the PowerGroup boundaries along the sector's edges, on the OUTSIDE
	// @Copypasta The code for all this is really repetitive, but I'm not sure how to factor it together nicely.

	// - Step 4.1: Left edge
	if (sector->base->bounds.x > 0)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relX = 0;
		for (s32 relY = 0;
			relY < sector->base->bounds.h;
			relY++)
		{
			u8 tilePGId = sector->tilePowerGroup[relY][relX];

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
				currentBoundary->x = sector->base->bounds.x - 1;
				currentBoundary->y = sector->base->bounds.y + relY;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.2: Right edge
	if (sector->base->bounds.x + sector->base->bounds.w < city->width)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relX = sector->base->bounds.w-1;
		for (s32 relY = 0;
			relY < sector->base->bounds.h;
			relY++)
		{
			u8 tilePGId = sector->tilePowerGroup[relY][relX];

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
				currentBoundary->x = sector->base->bounds.x + sector->base->bounds.w;
				currentBoundary->y = sector->base->bounds.y + relY;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.3: Top edge
	if (sector->base->bounds.y > 0)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relY = 0;
		for (s32 relX = 0;
			relX < sector->base->bounds.w;
			relX++)
		{
			u8 tilePGId = sector->tilePowerGroup[relY][relX];

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
				currentBoundary->x = sector->base->bounds.x + relX;
				currentBoundary->y = sector->base->bounds.y - 1;
				currentBoundary->w = 1;
				currentBoundary->h = 1;
			}
		}
	}

	// - Step 4.4: Bottom edge
	if (sector->base->bounds.y + sector->base->bounds.h < city->height)
	{
		u8 currentPGId = 0;
		Rect2I *currentBoundary = null;

		s32 relY = sector->base->bounds.h-1;
		for (s32 relX = 0;
			relX < sector->base->bounds.w;
			relX++)
		{
			u8 tilePGId = sector->tilePowerGroup[relY][relX];

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
				currentBoundary->x = sector->base->bounds.x + relX;
				currentBoundary->y = sector->base->bounds.y + sector->base->bounds.h;
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
		PowerSector *sector = powerLayer->sectors + getSectorIndexAtTilePos(bounds.x, bounds.y, powerLayer->sectorsX);
		bounds = intersectRelative(bounds, sector->base->bounds);

		s32 lastPowerGroupIndex = -1;

		// TODO: @Speed We could probably just do 1 loop because the bounds rect is only 1-wide in one dimension!
		for (s32 relY = bounds.y; relY < bounds.y + bounds.h; relY++)
		{
			for (s32 relX = bounds.x; relX < bounds.x + bounds.w; relX++)
			{
				s32 powerGroupIndex = sector->tilePowerGroup[relY][relX];
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
		sectorIndex < powerLayer->sectorCount;
		sectorIndex++)
	{
		PowerSector *sector = powerLayer->sectors + sectorIndex;

		recalculateSectorPowerGroups(city, sector);
	}

	// NB: All power groups are on networkID=0 right now, because they all got reconstructed in the above loop.
	// At some point we'll have to manually set that to 0, if we want to recalculate the global networks without
	// recalculating every individual sector.

	// Flood-fill networks of PowerGroups by walking the boundaries
	for (s32 sectorIndex = 0;
		sectorIndex < powerLayer->sectorCount;
		sectorIndex++)
	{
		PowerSector *sector = powerLayer->sectors + sectorIndex;

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
		sectorIndex < layer->sectorCount;
		sectorIndex++)
	{
		PowerSector *sector = layer->sectors + sectorIndex;
		updateSectorPowerValues(sector);
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
