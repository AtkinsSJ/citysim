#include "zone.h"

void initZoneLayer(MemoryArena *memoryArena, ZoneLayer *zoneLayer, s32 tileCount)
{
	zoneLayer->tiles = PushArray(memoryArena, ZoneType, tileCount);

	initChunkedArray(&zoneLayer->rGrowableBuildings, memoryArena, 256);
	initChunkedArray(&zoneLayer->emptyRZones,        memoryArena, 256);
	initChunkedArray(&zoneLayer->filledRZones,       memoryArena, 256);

	initChunkedArray(&zoneLayer->cGrowableBuildings, memoryArena, 256);
	initChunkedArray(&zoneLayer->emptyCZones,        memoryArena, 256);
	initChunkedArray(&zoneLayer->filledCZones,       memoryArena, 256);

	initChunkedArray(&zoneLayer->iGrowableBuildings, memoryArena, 256);
	initChunkedArray(&zoneLayer->emptyIZones,        memoryArena, 256);
	initChunkedArray(&zoneLayer->filledIZones,       memoryArena, 256);
}

bool canZoneTile(City *city, ZoneType zoneType, s32 x, s32 y)
{
	if (!tileExists(city, x, y)) return false;

	s32 tile = tileIndex(city, x, y);

	TerrainDef *tDef = get(&terrainDefs, terrainAt(city, x, y).type);
	if (!tDef->canBuildOn) return false;

	if (city->tileBuildings[tile] != 0) return false;

	// Ignore tiles that are already this zone!
	if (city->zoneLayer.tiles[tile] == zoneType) return false;

	return true;
}

s32 calculateZoneCost(City *city, ZoneType zoneType, Rect2I area)
{
	ZoneDef zoneDef = zoneDefs[zoneType];

	s32 total = 0;

	// For now, you can only zone free tiles, where there are no buildings or obstructions.
	// You CAN zone over other zones though. Just, not if something has already grown in that
	// zone tile.
	// At some point we'll probably want to change this, because my least-favourite thing
	// about SC3K is that clearing a built-up zone means demolishing the buildings then quickly
	// switching to the de-zone tool before something else pops up in the free space!
	// - Sam, 13/12/2018

	for (int y=0; y<area.h; y++)
	{
		for (int x=0; x<area.w; x++)
		{
			if (canZoneTile(city, zoneType, area.x + x, area.y + y))
			{
				total += zoneDef.costPerTile;
			}
		}
	}

	return total;
}

static ChunkedArray<V2I> *getEmptyZonesArray(ZoneLayer *layer, ZoneType zoneType)
{
	ChunkedArray<V2I> *emptyZonesArray = null;
	switch (zoneType)
	{
		case Zone_Residential:  emptyZonesArray = &layer->emptyRZones; break;
		case Zone_Commercial:   emptyZonesArray = &layer->emptyCZones; break;
		case Zone_Industrial:   emptyZonesArray = &layer->emptyIZones; break;
	}

	return emptyZonesArray;
}

static ChunkedArray<V2I> *getFilledZonesArray(ZoneLayer *layer, ZoneType zoneType)
{
	ChunkedArray<V2I> *filledZonesArray = null;
	switch (zoneType)
	{
		case Zone_Residential:  filledZonesArray = &layer->filledRZones; break;
		case Zone_Commercial:   filledZonesArray = &layer->filledCZones; break;
		case Zone_Industrial:   filledZonesArray = &layer->filledIZones; break;
	}

	return filledZonesArray;
}

void placeZone(UIState *uiState, City *city, ZoneType zoneType, Rect2I area, bool chargeMoney)
{
	if (chargeMoney)
	{
		s32 cost = calculateZoneCost(city, zoneType, area);
		if (!canAfford(city, cost))
		{
			pushUiMessage(uiState, stringFromChars("Not enough money to zone this."));
			return;
		}
		else
		{
			spend(city, cost);
		}
	}
	
	ChunkedArray<V2I> *emptyZonesArray = getEmptyZonesArray(&city->zoneLayer, zoneType);

	for (int y=0; y<area.h; y++) {
		for (int x=0; x<area.w; x++) {
			V2I pos = v2i(area.x + x, area.y + y);
			if (canZoneTile(city, zoneType, pos.x, pos.y))
			{
				s32 tile = tileIndex(city, pos.x, pos.y);

				ZoneType oldZone = city->zoneLayer.tiles[tile];

				ChunkedArray<V2I> *oldEmptyZonesArray = getEmptyZonesArray(&city->zoneLayer, oldZone);
				if (oldEmptyZonesArray)
				{
					// We need to *remove* the position
					findAndRemove(oldEmptyZonesArray, pos);
				}

				if (emptyZonesArray)
				{
					append(emptyZonesArray, pos);
				}

				city->zoneLayer.tiles[tile] = zoneType;
			}
		}
	}

	// Zones carry power!
	recalculatePowerConnectivity(city);
}

void markZonesAsEmpty(City *city, Rect2I footprint)
{
	// NB: We're assuming there's only one zone type within the footprint,
	// because we don't support buildings that can grow in multiple different zones.
	ZoneType zoneType = getZoneAt(city, footprint.x, footprint.y);
	ChunkedArray<V2I> *emptyZonesArray  = getEmptyZonesArray( &city->zoneLayer, zoneType);

	if (emptyZonesArray)
	{
		ChunkedArray<V2I> *filledZonesArray = getFilledZonesArray(&city->zoneLayer, zoneType);

		for (s32 y=0; y < footprint.h; y++)
		{
			for (s32 x=0; x < footprint.w; x++)
			{
				// Mark the zone as empty
				V2I pos = v2i(footprint.x+x, footprint.y+y);
				findAndRemove(filledZonesArray, pos);
				append(emptyZonesArray, pos);
			}
		}
	}
}

void growZoneBuilding(City *city, BuildingDef *def, Rect2I footprint)
{
	/* 
		We make some assumptions here, because some building features don't make sense
		for zoned buildings, and we already checked the footprint with isZoneAcceptable().
		The assumptions are:
		 - There is no building or other obstacle already overlapping the footprint
		 - There is only one zone type across the entire footprint
		 - This building doesn't affect the paths layer
		 - This building doesn't affect the power layer (because power is carried by the zone)
		 - This building doesn't remove the existing zone
		 - This building doesn't need to affect adjacent building textures
	 */

	u32 buildingID = city->buildings.count;
	Building *building = appendBlank(&city->buildings);
	building->typeID = def->typeID;
	building->footprint = footprint;

	ZoneType zoneType = getZoneAt(city, footprint.x, footprint.y);
	ChunkedArray<V2I> *emptyZonesArray  = getEmptyZonesArray( &city->zoneLayer, zoneType);
	ChunkedArray<V2I> *filledZonesArray = getFilledZonesArray(&city->zoneLayer, zoneType);

	ASSERT(emptyZonesArray && filledZonesArray, "Attempting to grow a building in a zone with no empty/filled zones arrays.");

	for (s32 y=0; y<building->footprint.h; y++)
	{
		for (s32 x=0; x<building->footprint.w; x++)
		{
			V2I pos = v2i(building->footprint.x+x, building->footprint.y+y);
			s32 tile = tileIndex(city, pos.x, pos.y);

			// Put the building there
			city->tileBuildings[tile] = buildingID;

			// Mark the zone as filled
			findAndRemove(emptyZonesArray, pos);
			append(filledZonesArray, pos);
		}
	}

	// TODO: Properly calculate occupancy!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	city->totalResidents += def->residents;
	city->totalJobs += def->jobs;

	updateBuildingTexture(city, building, def);
}

static bool isZoneAcceptable(City *city, ZoneType zoneType, s32 x, s32 y)
{
	// For now, we'll ignore zoneType and assume the same requirements for all zones, but later we'll want to make distance-to-road etc variable for them.
	// TODO: Different requirements per ZoneType

	bool isAcceptable = true;

	if (getZoneAt(city, x, y) != zoneType)
	{
		isAcceptable = false;
	}
	else if (getBuildingAtPosition(city, x, y) != null)
	{
		isAcceptable = false;
	}

	// TODO: Distance to road!
	// TODO: Power requirements!

	return isAcceptable;
}

void growSomeZoneBuildings(City *city)
{
	ZoneLayer *layer = &city->zoneLayer;
	Random *random = city->gameRandom;

	// Residential
	if (city->residentialDemand > 0)
	{
		s32 remainingDemand = city->residentialDemand;
		s32 minimumDemand = city->residentialDemand / 20; // Stop when we're below 20% of the original demand

		while ((layer->emptyRZones.itemCount > 0) && (remainingDemand > minimumDemand))
		{
			// Find a valid res zone
			// TODO: Better selection than just a random one
			bool foundAZone = false;
			V2I zonePos = {};
			for (auto it = iterate(&layer->emptyRZones, randomInRange(random, layer->emptyRZones.itemCount));
				!it.isDone;
				next(&it))
			{
				V2I aPos = getValue(it);

				if (isZoneAcceptable(city, Zone_Residential, aPos.x, aPos.y))
				{
					zonePos = aPos;
					foundAZone = true;
					break;
				}
			}

			if (!foundAZone)
			{
				// There are empty zones, but none meet our requirements, so stop trying to grow anything
				break;
			}
			
			// Produce a rectangle of the contiguous empty zone area around that point,
			// so we can fit larger buildings there if possible.
			Rect2I zoneFootprint = irectXYWH(zonePos.x, zonePos.y, 1, 1);
			{
				// Gradually expand from zonePos outwards, checking if there is available, zoned space

				bool tryX = randomBool(random);
				bool positive = randomBool(random);

				while (true)
				{
					bool canExpand = true;
					if (tryX)
					{
						s32 x = positive ? (zoneFootprint.x + zoneFootprint.w) : (zoneFootprint.x - 1);

						for (s32 y = zoneFootprint.y;
							y < zoneFootprint.y + zoneFootprint.h;
							y++)
						{
							if (!isZoneAcceptable(city, Zone_Residential, x, y))
							{
								canExpand = false;
								break;
							}
						}

						if (canExpand)
						{
							zoneFootprint.w++;
							if (!positive) zoneFootprint.x--;
						}
					}
					else // expand in y
					{
						s32 y = positive ? (zoneFootprint.y + zoneFootprint.h) : (zoneFootprint.y - 1);

						for (s32 x = zoneFootprint.x;
							x < zoneFootprint.x + zoneFootprint.w;
							x++)
						{
							if (!isZoneAcceptable(city, Zone_Residential, x, y))
							{
								canExpand = false;
								break;
							}
						}

						if (canExpand)
						{
							zoneFootprint.h++;
							if (!positive) zoneFootprint.y--;
						}
					}

					// As soon as we fail to expand in a direction, just stop
					if (!canExpand) break;

					// Maximum size, don't bother trying more than 5x5
					if (zoneFootprint.w >=5 && zoneFootprint.h >= 5) break;

					tryX = !tryX; // Alternate x and y
					positive = randomBool(random); // random direction to expand in
				}
			}

			// Pick a building def that fits the space and is not more than 10% more than the remaining demand
			BuildingDef *buildingDef = null;
			s32 maximumResidents = (s32) ((f32)remainingDemand * 1.1f);

			// Choose a random building, then carry on checking buildings until one is acceptable
			for (auto it = iterate(&layer->rGrowableBuildings, randomInRange(random, layer->rGrowableBuildings.itemCount));
				!it.isDone;
				next(&it))
			{
				BuildingDef *aDef = get(&buildingDefs, getValue(it));

				// Cap residents
				if (aDef->residents > maximumResidents) continue;

				// Cap based on size
				if (aDef->width > zoneFootprint.w || aDef->height > zoneFootprint.h) continue;
				
				buildingDef = aDef;
				break;
			}

			if (buildingDef)
			{
				// Place it!
				// TODO: Right now this places at the top-left of the zoneFootprint... probably want to be better than that.
				Rect2I footprint = irectPosDim(zoneFootprint.pos, buildingDef->size);
				growZoneBuilding(city, buildingDef, footprint);
				remainingDemand -= buildingDef->residents;
			}
			else
			{
				// We failed to find a building def, so we should probably stop trying to build things!
				// Otherwise, we'll get stuck in an infinite loop
				break;
			}
		}
	}

	// TODO: Commercial

	// TODO: Industrial

	// TODO: Other zones maybe???
}

void refreshZoneGrowableBuildingLists(ZoneLayer *zoneLayer)
{
	clear(&zoneLayer->rGrowableBuildings);
	clear(&zoneLayer->cGrowableBuildings);
	clear(&zoneLayer->iGrowableBuildings);

	for (auto it = iterate(&buildingDefs); !it.isDone; next(&it))
	{
		BuildingDef *def = get(it);

		switch(def->growsInZone)
		{
			case Zone_Residential:  append(&zoneLayer->rGrowableBuildings, def->typeID); break;
			case Zone_Commercial:   append(&zoneLayer->cGrowableBuildings, def->typeID); break;
			case Zone_Industrial:   append(&zoneLayer->iGrowableBuildings, def->typeID); break;
		}
	}

	logInfo("Loaded {0} R, {1} C and {2} I growable buildings.", {
		formatInt(zoneLayer->rGrowableBuildings.itemCount),
		formatInt(zoneLayer->cGrowableBuildings.itemCount),
		formatInt(zoneLayer->iGrowableBuildings.itemCount)
	});
}