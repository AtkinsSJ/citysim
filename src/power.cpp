#pragma once

void initialisePowerLayer(MemoryArena *gameArena, PowerLayer *layer)
{
	initChunkedArray(&layer->groups, gameArena, 64);
}

void setPowerGroup(City *city, s32 x, s32 y, s32 value)
{
	Sector *sector = sectorAtTilePos(city, x, y);
	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		sector->tilePowerGroup[relY][relX] = value;
	}
}

void floodFillPowerConnectivity(City *city, s32 x, s32 y, s32 fillValue)
{
	setPowerGroup(city, x, y, fillValue);

	if (powerGroupAt(city, x-1, y) == -1)
	{
		floodFillPowerConnectivity(city, x-1, y, fillValue);
	}
	
	if (powerGroupAt(city, x+1, y) == -1)
	{
		floodFillPowerConnectivity(city, x+1, y, fillValue);
	}

	if (powerGroupAt(city, x, y+1) == -1)
	{
		floodFillPowerConnectivity(city, x, y+1, fillValue);
	}

	if (powerGroupAt(city, x, y-1) == -1)
	{
		floodFillPowerConnectivity(city, x, y-1, fillValue);
	}
}

/*
 * @Performance This is a really naive approach to recalculating power. Every time ANYTHING
 * changes, we start over and calculate the whole city's power grid and production/comsumption
 * from scratch!!! There should be lots of shortcuts we can take.
 */
void recalculatePowerConnectivity(City *city)
{
	DEBUG_FUNCTION();

	// This is a flood fill.
	// First, normalise things so path tiles are -1, others are 0
	// Then, iterate over the tiles and flood fill from each -1 value.

	// Reset things to 0/-1
	for (s32 sectorIndex = 0;
		sectorIndex < city->sectorCount;
		sectorIndex++)
	{
		Sector *sector = city->sectors + sectorIndex;

		for (s32 relY = 0;
			relY < sector->bounds.h;
			relY++)
		{
			for (s32 relX = 0;
				relX < sector->bounds.w;
				relX++)
			{
				bool tileCarriesPower = false;
				ZoneType zone = sector->tileZone[relY][relX];
				if (zoneDefs[zone].carriesPower)
				{
					tileCarriesPower = true;
				}
				else
				{
					// TODO: This is super inefficient
					Building *building = getBuildingAtPosition(city, sector->bounds.x + relX, sector->bounds.y + relY);
					if (building != null)
					{
						if (get(&buildingDefs, building->typeID)->carriesPower)
						{
							tileCarriesPower = true;
						}
					}
				}

				sector->tilePowerGroup[relY][relX] = tileCarriesPower ? -1 : 0;
			}
		}
	}

	clear(&city->powerLayer.groups);
	appendBlank(&city->powerLayer.groups); // index 0 is nothing
	city->powerLayer.combined = {};

	// Find -1 tiles
	// TODO: Calculate this per-sector!
	for (s32 y=0, tileIndex = 0; y<city->height; y++)
	{
		for (s32 x=0; x<city->width; x++, tileIndex++)
		{
			if (powerGroupAt(city, x, y) == -1)
			{
				// Flood fill from here!
				floodFillPowerConnectivity(city, x, y, truncate32(city->powerLayer.groups.count));
				PowerGroup *newGroup = appendBlank(&city->powerLayer.groups);
				newGroup->production = 0;
				newGroup->consumption = 0;
			}
		}
	}

	// Find all power production/consumption buildings and add them up
	// TODO: some kind of building-type query would probably be better?
	for (s32 sectorIndex = 0;
		sectorIndex < city->sectorCount;
		sectorIndex++)
	{
		Sector *sector = city->sectors + sectorIndex;

		for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
		{
			Building *building = get(it);
			BuildingDef *def = get(&buildingDefs, building->typeID);

			if (def->power != 0)
			{
				PowerGroup *powerGroup = get(&city->powerLayer.groups, powerGroupAt(city, building->footprint.x, building->footprint.y));

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

	for (auto it = iterate(&city->powerLayer.groups, 1, false); !it.isDone; next(&it))
	{
		PowerGroup *group = get(it);
		city->powerLayer.combined.production  += group->production;
		city->powerLayer.combined.consumption += group->consumption;
	}
}