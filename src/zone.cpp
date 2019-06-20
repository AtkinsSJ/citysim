#pragma once

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena)
{
	initChunkPool(&zoneLayer->zoneLocationsChunkPool, gameArena, 256);

	initChunkedArray(&zoneLayer->emptyRZones,  &zoneLayer->zoneLocationsChunkPool);
	initChunkedArray(&zoneLayer->filledRZones, &zoneLayer->zoneLocationsChunkPool);

	initChunkedArray(&zoneLayer->emptyCZones,  &zoneLayer->zoneLocationsChunkPool);
	initChunkedArray(&zoneLayer->filledCZones, &zoneLayer->zoneLocationsChunkPool);

	initChunkedArray(&zoneLayer->emptyIZones,  &zoneLayer->zoneLocationsChunkPool);
	initChunkedArray(&zoneLayer->filledIZones, &zoneLayer->zoneLocationsChunkPool);

	initSectorGrid(&zoneLayer->sectors, gameArena, city->width, city->height, 16);
	for (s32 sectorIndex = 0; sectorIndex < zoneLayer->sectors.count; sectorIndex++)
	{
		ZoneSector *sector = zoneLayer->sectors.sectors + sectorIndex;

		sector->tileZone = PushArray(gameArena, ZoneType, sector->bounds.w * sector->bounds.h);
	}
}

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;
	ZoneSector *sector = getSectorAtTilePos(&city->zoneLayer.sectors, x, y);

	if (sector != null)
	{
		s32 relX = x - sector->bounds.x;
		s32 relY = y - sector->bounds.y;

		result = *getSectorTile(sector, sector->tileZone, relX, relY);
	}

	return result;
}

bool canZoneTile(City *city, ZoneType zoneType, s32 x, s32 y)
{
	DEBUG_FUNCTION();
	
	if (!tileExists(city, x, y)) return false;

	TerrainDef *tDef = get(&terrainDefs, getTerrainAt(city, x, y)->type);
	if (!tDef->canBuildOn) return false;

	if (getBuildingAtPosition(city, x, y) != null) return false;

	// Ignore tiles that are already this zone!
	if (getZoneAt(city, x, y) == zoneType) return false;

	return true;
}

s32 calculateZoneCost(City *city, ZoneType zoneType, Rect2I area)
{
	DEBUG_FUNCTION();

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

void drawZones(ZoneLayer *zoneLayer, Renderer *renderer, Rect2I visibleArea, s32 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	s32 zoneType = -1;
	V4 zoneColor = {};

	Rect2I visibleSectors = getSectorsCovered(&zoneLayer->sectors, visibleArea);
	for (s32 sY = visibleSectors.y;
		sY < visibleSectors.y + visibleSectors.h;
		sY++)
	{
		for (s32 sX = visibleSectors.x;
			sX < visibleSectors.x + visibleSectors.w;
			sX++)
		{
			ZoneSector *sector = getSector(&zoneLayer->sectors, sX, sY);

			Rect2I relArea = intersectRelative(visibleArea, sector->bounds);
			for (s32 relY=relArea.y;
				relY < relArea.y + relArea.h;
				relY++)
			{
				spriteBounds.y = (f32)(sector->bounds.y + relY);

				for (s32 relX=relArea.x;
					relX < relArea.x + relArea.w;
					relX++)
				{
					ZoneType zone = *getSectorTile(sector, sector->tileZone, relX, relY);
					if (zone != Zone_None)
					{
						if (zone != zoneType)
						{
							zoneType = zone;
							zoneColor = zoneDefs[zoneType].color;
						}

						spriteBounds.x = (f32)(sector->bounds.x + relX);

						drawRect(&renderer->worldBuffer, spriteBounds, -900.0f, shaderID, zoneColor);
					}
				}
			}
		}
	}
}

void placeZone(City *city, ZoneType zoneType, Rect2I area)
{
	DEBUG_FUNCTION();

	ZoneLayer *zoneLayer = &city->zoneLayer;
	
	ChunkedArray<V2I> *emptyZonesArray = getEmptyZonesArray(zoneLayer, zoneType);

	// TODO: @Speed Invert how zone location removal is done.
	// Rather than doing a linear search for each tile, we could instead loop once through the zones
	// list and remove any that are within the rectangle. (If we can edit an array while iterating it...)
	// Once we have sectors, changes like this can be done by looping through the affected sectors and
	// then doing the check in each one.
	for (int y=0; y<area.h; y++) {
		for (int x=0; x<area.w; x++) {
			V2I pos = v2i(area.x + x, area.y + y);
			if (canZoneTile(city, zoneType, pos.x, pos.y))
			{
				ZoneSector *sector = getSectorAtTilePos(&zoneLayer->sectors, pos.x, pos.y);
				s32 relX = pos.x - sector->bounds.x;
				s32 relY = pos.y - sector->bounds.y;
				ZoneType oldZone = *getSectorTile(sector, sector->tileZone, relX, relY);

				// URGHGGHGHHH THIS IS HORRRRRIBLE!
				// We're doing a linear search through the chunked array for EVERY TILE that's changed!
				// Then again, I'm not sure there's a good way to *not* do that while still having a
				// list of locations by zone type, which we use for spawning buildings. Maybe this is
				// actually slower overall than not having the empty-zones list at all.
				// In any case, it'll be a lot better once we have the city divided up into smaller
				// sections that manage their own caches.
				// - Sam, 03/06/2019

				ChunkedArray<V2I> *oldEmptyZonesArray = getEmptyZonesArray(zoneLayer, oldZone);
				if (oldEmptyZonesArray)
				{
					// We need to *remove* the position
					findAndRemove(oldEmptyZonesArray, pos);
				}

				if (emptyZonesArray)
				{
					append(emptyZonesArray, pos);
				}

				setSectorTile(sector, sector->tileZone, relX, relY, zoneType);
			}
		}
	}

	// Zones carry power!
	markPowerLayerDirty(&city->powerLayer, area);
}

void markZonesAsEmpty(City *city, Rect2I footprint)
{
	DEBUG_FUNCTION();
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
	DEBUG_FUNCTION();
	
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

	Building *building = addBuilding(city, def, footprint);

	ZoneType zoneType = getZoneAt(city, footprint.x, footprint.y);
	ChunkedArray<V2I> *emptyZonesArray  = getEmptyZonesArray( &city->zoneLayer, zoneType);
	ChunkedArray<V2I> *filledZonesArray = getFilledZonesArray(&city->zoneLayer, zoneType);

	ASSERT(emptyZonesArray && filledZonesArray, "Attempting to grow a building in a zone with no empty/filled zones arrays.");

	for (s32 y=0; y<building->footprint.h; y++)
	{
		for (s32 x=0; x<building->footprint.w; x++)
		{
			V2I pos = v2i(building->footprint.x+x, building->footprint.y+y);

			// Mark the zone as filled
			findAndRemove(emptyZonesArray, pos);
			append(filledZonesArray, pos);
		}
	}

	city->totalResidents += def->residents;
	city->totalJobs += def->jobs;

	updateBuildingTexture(city, building, def);
}

static bool isZoneAcceptable(City *city, ZoneType zoneType, s32 x, s32 y)
{
	DEBUG_FUNCTION();
	
	ZoneDef def = zoneDefs[zoneType];

	bool isAcceptable = true;

	if (getZoneAt(city, x, y) != zoneType)
	{
		isAcceptable = false;
	}
	else if (getBuildingAtPosition(city, x, y) != null)
	{
		isAcceptable = false;
	}
	else if (calculateDistanceToRoad(city, x, y, def.maximumDistanceToRoad) > def.maximumDistanceToRoad)
	{
		isAcceptable = false;
	}

	// TODO: Power requirements!

	return isAcceptable;
}

void growSomeZoneBuildings(City *city)
{
	DEBUG_FUNCTION();

	ZoneLayer *layer = &city->zoneLayer;
	Random *random = city->gameRandom;

	// Residential
	if (city->residentialDemand > 0)
	{
		s32 remainingDemand = city->residentialDemand;
		s32 minimumDemand = city->residentialDemand / 20; // Stop when we're below 20% of the original demand

		s32 maxRBuildingDim = buildingCatalogue.maxRBuildingDim;

		while ((layer->emptyRZones.count > 0) && (remainingDemand > minimumDemand))
		{
			// Find a valid res zone
			// TODO: Better selection than just a random one
			bool foundAZone = false;
			V2I zonePos = {};
			{
				DEBUG_BLOCK("growSomeZoneBuildings - find a valid zone");
				for (auto it = iterate(&layer->emptyRZones, randomInRange(random, truncate32(layer->emptyRZones.count)));
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
				DEBUG_BLOCK("growSomeZoneBuildings - expand rect");
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

					// No need to grow bigger than the largest possible building!
					// (Unless at some point we do "batches" of buildings like SC4 does)
					if (zoneFootprint.w >= maxRBuildingDim && zoneFootprint.h >= maxRBuildingDim) break;

					tryX = !tryX; // Alternate x and y
					positive = randomBool(random); // random direction to expand in
				}
			}

			// Pick a building def that fits the space and is not more than 10% more than the remaining demand
			BuildingDef *buildingDef = null;
			s32 maximumResidents = (s32) ((f32)remainingDemand * 1.1f);

			{
				DEBUG_BLOCK("growSomeZoneBuildings - pick a building def");

				// Choose a random building, then carry on checking buildings until one is acceptable
				ChunkedArray<BuildingDef *> *rGrowableBuildings = getRGrowableBuildings();
				for (auto it = iterate(rGrowableBuildings, randomInRange(random, truncate32(rGrowableBuildings->count)));
					!it.isDone;
					next(&it))
				{
					BuildingDef *aDef = getValue(it);

					// Cap residents
					if (aDef->residents > maximumResidents) continue;

					// Cap based on size
					if (aDef->width > zoneFootprint.w || aDef->height > zoneFootprint.h) continue;
					
					buildingDef = aDef;
					break;
				}
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
