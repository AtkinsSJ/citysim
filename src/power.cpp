#pragma once

void initialisePowerLayer(MemoryArena *gameArena, PowerLayer *layer)
{
	initChunkedArray(&layer->groups, gameArena, 64);
}

void floodFillSectorPowerGroup(Sector *sector, s32 x, s32 y, u8 fillValue)
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

	if ((x < sector->bounds.w-1) && (sector->tilePowerGroup[y][x+1] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x+1, y, fillValue);
	}

	if ((y > 0) && (sector->tilePowerGroup[y-1][x] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y-1, fillValue);
	}
	
	if ((y < sector->bounds.h-1) && (sector->tilePowerGroup[y+1][x] == POWER_GROUP_UNKNOWN))
	{
		floodFillSectorPowerGroup(sector, x, y+1, fillValue);
	}
}

inline void setRectPowerGroupUnknown(Sector *sector, Rect2I area)
{
	Rect2I relArea = cropRectangleToRelativeWithinSector(area, sector);

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

void recalculateSectorPowerGroups(City *city, Sector *sector)
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
	for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
	{
		Building *building = get(it);
		BuildingDef *def = get(&buildingDefs, building->typeID);

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
			if (sector->tilePowerGroup[relY][relX] == POWER_GROUP_UNKNOWN) continue;

			ZoneType zone = sector->tileZone[relY][relX];
			if (zoneDefs[zone].carriesPower)
			{
				sector->tilePowerGroup[relY][relX] = POWER_GROUP_UNKNOWN;
			}
			else
			{
				Building *building = getBuildingAtPosition(city, sector->bounds.x + relX, sector->bounds.y + relY);
				if (building != null && get(&buildingDefs, building->typeID)->carriesPower)
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
	for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
	{
		Building *building = get(it);
		BuildingDef *def = get(&buildingDefs, building->typeID);

		if (def->power != 0)
		{
			u8 powerGroupIndex = sector->tilePowerGroup[building->footprint.y - sector->bounds.y][building->footprint.x - sector->bounds.x];
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

/*
 * TODO: Recalculate individual Sectors as needed, instead of recalculating EVERY ONE whenever anything changes!
 * Still need to recalculate the city-wide networks though.
 */
void recalculatePowerConnectivity(City *city)
{
	DEBUG_FUNCTION();

	// Recalculate each sector
	for (s32 sectorIndex = 0;
		sectorIndex < city->sectorCount;
		sectorIndex++)
	{
		Sector *sector = city->sectors + sectorIndex;

		recalculateSectorPowerGroups(city, sector);
	}

	// Flood-fill networks of PowerGroups by walking the boundaries

#if 0 // Old code

	// This is a flood fill.
	// First, normalise things so path tiles are -1, others are 0
	// Then, iterate over the tiles and flood fill from each -1 value.

	// Reset things to 0/-1
	// for (s32 sectorIndex = 0;
	// 	sectorIndex < city->sectorCount;
	// 	sectorIndex++)
	// {
	// 	Sector *sector = city->sectors + sectorIndex;

	// 	for (s32 relY = 0;
	// 		relY < sector->bounds.h;
	// 		relY++)
	// 	{
	// 		for (s32 relX = 0;
	// 			relX < sector->bounds.w;
	// 			relX++)
	// 		{
	// 			bool tileCarriesPower = false;
	// 			ZoneType zone = sector->tileZone[relY][relX];
	// 			if (zoneDefs[zone].carriesPower)
	// 			{
	// 				tileCarriesPower = true;
	// 			}
	// 			else
	// 			{
	// 				// TODO: This is super inefficient
	// 				Building *building = getBuildingAtPosition(city, sector->bounds.x + relX, sector->bounds.y + relY);
	// 				if (building != null)
	// 				{
	// 					if (get(&buildingDefs, building->typeID)->carriesPower)
	// 					{
	// 						tileCarriesPower = true;
	// 					}
	// 				}
	// 			}

	// 			sector->tilePowerGroup[relY][relX] = tileCarriesPower ? -1 : 0;
	// 		}
	// 	}
	// }

	// clear(&city->powerLayer.groups);
	// appendBlank(&city->powerLayer.groups); // index 0 is nothing
	// city->powerLayer.combined = {};

	// // Find -1 tiles
	// // TODO: Calculate this per-sector!
	// for (s32 y=0, tileIndex = 0; y<city->height; y++)
	// {
	// 	for (s32 x=0; x<city->width; x++, tileIndex++)
	// 	{
	// 		if (powerGroupAt(city, x, y) == -1)
	// 		{
	// 			// Flood fill from here!
	// 			floodFillPowerConnectivity(city, x, y, truncate32(city->powerLayer.groups.count));
	// 			PowerGroup *newGroup = appendBlank(&city->powerLayer.groups);
	// 			newGroup->production = 0;
	// 			newGroup->consumption = 0;
	// 		}
	// 	}
	// }

	// // Find all power production/consumption buildings and add them up
	// // TODO: some kind of building-type query would probably be better?
	// for (s32 sectorIndex = 0;
	// 	sectorIndex < city->sectorCount;
	// 	sectorIndex++)
	// {
	// 	Sector *sector = city->sectors + sectorIndex;

	// 	for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
	// 	{
	// 		Building *building = get(it);
	// 		BuildingDef *def = get(&buildingDefs, building->typeID);

	// 		if (def->power != 0)
	// 		{
	// 			PowerGroup *powerGroup = get(&city->powerLayer.groups, powerGroupAt(city, building->footprint.x, building->footprint.y));

	// 			if (def->power > 0)
	// 			{
	// 				powerGroup->production += def->power;
	// 			}
	// 			else
	// 			{
	// 				powerGroup->consumption -= def->power;
	// 			}
	// 		}
	// 	}
	// }

	// for (auto it = iterate(&city->powerLayer.groups, 1, false); !it.isDone; next(&it))
	// {
	// 	PowerGroup *group = get(it);
	// 	city->powerLayer.combined.production  += group->production;
	// 	city->powerLayer.combined.consumption += group->consumption;
	// }
#endif
}