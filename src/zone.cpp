#pragma once

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena)
{
	zoneLayer->tileZone = allocateMultiple<ZoneType>(gameArena, city->width * city->height);

	initSectorGrid(&zoneLayer->sectors, gameArena, city->width, city->height, 16);
	s32 sectorCount = getSectorCount(&zoneLayer->sectors);

	for (s32 zoneType = 0; zoneType < ZoneCount; zoneType++)
	{
		initBitArray(&zoneLayer->sectorsWithZones[zoneType],      gameArena, sectorCount);
		initBitArray(&zoneLayer->sectorsWithEmptyZones[zoneType], gameArena, sectorCount);
	}

	for (s32 sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++)
	{
		ZoneSector *sector = &zoneLayer->sectors.sectors[sectorIndex];

		sector->zoneSectorFlags = 0;
	}
}

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;
	if (tileExists(city, x, y))
	{
		result = *getTile(city, city->zoneLayer.tileZone, x, y);
	}

	return result;
}

inline s32 calculateZoneCost(CanZoneQuery *query)
{
	return query->zoneableTilesCount * query->zoneDef->costPerTile;
}

CanZoneQuery *queryCanZoneTiles(City *city, ZoneType zoneType, Rect2I bounds)
{
	DEBUG_FUNCTION_T(DCDT_Highlight);
	// Assumption made: `bounds` is a valid rectangle within the city's bounds

	// Allocate the Query struct
	CanZoneQuery *query = null;
	s32 tileCount = areaOf(bounds);
	smm structSize = sizeof(CanZoneQuery) + (tileCount * sizeof(query->tileCanBeZoned[0]));
	u8 *memory = (u8*) allocate(tempArena, structSize);
	query = (CanZoneQuery *) memory;
	query->tileCanBeZoned = (u8 *) (memory + sizeof(CanZoneQuery));
	query->bounds = bounds;
	query->zoneDef = &zoneDefs[zoneType];

	// Actually do the calculation

	// For now, you can only zone free tiles, where there are no buildings or obstructions.
	// You CAN zone over other zones though. Just, not if something has already grown in that
	// zone tile.
	// At some point we'll probably want to change this, because my least-favourite thing
	// about SC3K is that clearing a built-up zone means demolishing the buildings then quickly
	// switching to the de-zone tool before something else pops up in the free space!
	// - Sam, 13/12/2018

	for (s32 y = bounds.y;
		y < bounds.y + bounds.h;
		y++)
	{
		for (s32 x = bounds.x;
			x < bounds.x + bounds.w;
			x++)
		{
			bool canZone = true;

			// Ignore tiles that are already this zone!
			if (getZoneAt(city, x, y) == zoneType)
			{
				canZone = false;
			}
			// Terrain must be buildable
			// @Speed: URGH this terrain lookup for every tile is nasty!
			else if (!get(&terrainDefs, getTerrainAt(city, x, y)->type)->canBuildOn)
			{
				canZone = false;
			}
			// Tile must be empty
			else if (buildingExistsAtPosition(city, x, y))
			{
				canZone = false;
			}

			// Pos relative to our query
			s32 qX = x - bounds.x;
			s32 qY = y - bounds.y;
			query->tileCanBeZoned[(qY * bounds.w) + qX] = canZone ? 1 : 0;
			if (canZone) query->zoneableTilesCount++;
		}
	}

	return query;
}

inline bool canZoneTile(CanZoneQuery *query, s32 x, s32 y)
{
	ASSERT(contains(query->bounds, x, y));
	s32 qX = x - query->bounds.x;
	s32 qY = y - query->bounds.y;

	return query->tileCanBeZoned[(qY * query->bounds.w) + qX] != 0;
}

void drawZones(City *city, Rect2I visibleArea, s8 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	s32 zoneType = -1;
	V4 zoneColor = {};

	// TODO: @Speed: areaOf() is a poor heuristic! It's safely >= the actual value, but it would be better to
	// actually see how many there are. Though that'd be a double-iteration, unless we keep a cached count.
	DrawRectsGroup *group = beginRectsGroupUntextured(&renderer->worldBuffer, shaderID, areaOf(visibleArea));

	for (s32 y = visibleArea.y;
		y < visibleArea.y + visibleArea.h;
		y++)
	{
		spriteBounds.y = (f32) y;
		for (s32 x = visibleArea.x;
			x < visibleArea.x + visibleArea.w;
			x++)
		{
			ZoneType zone = getZoneAt(city, x, y);
			if (zone != Zone_None)
			{
				if (zone != zoneType)
				{
					zoneType = zone;
					zoneColor = zoneDefs[zoneType].color;
				}

				spriteBounds.x = (f32) x;

				addUntexturedRect(group, spriteBounds, zoneColor);
			}
		}
	}

	endRectsGroup(group);
}

void placeZone(City *city, ZoneType zoneType, Rect2I area)
{
	DEBUG_FUNCTION();

	ZoneLayer *zoneLayer = &city->zoneLayer;

	for (s32 y = area.y;
		y < area.y + area.h;
		y++)
	{
		for (s32 x = area.x;
			x < area.x + area.w;
			x++)
		{
			// Ignore tiles that are already this zone!
			if ((*getTile(city, zoneLayer->tileZone, x, y) != zoneType)
			// Terrain must be buildable
			// @Speed: URGH this terrain lookup for every tile is nasty!
			&& (get(&terrainDefs, getTerrainAt(city, x, y)->type)->canBuildOn)
			&& (!buildingExistsAtPosition(city, x, y)))
			{
				setTile(city, zoneLayer->tileZone, x, y, zoneType);
			}
		}
	}

	// TODO: mark the affected zone sectors as dirty

	// Zones carry power!
	markPowerLayerDirty(&city->powerLayer, area);
}

void markZonesAsEmpty(City * /*city*/, Rect2I /*footprint*/)
{
	DEBUG_FUNCTION();
	
	// This now doesn't do anything but I'm keeping it because we probably want dirty rects/local updates later.
}

void updateZoneLayer(City *city, ZoneLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	for (s32 zoneType = 0; zoneType < ZoneCount; zoneType++)
	{
		clearBits(&layer->sectorsWithZones[zoneType]);
		clearBits(&layer->sectorsWithEmptyZones[zoneType]);
	}

	for (s32 sectorIndex = 0;
		sectorIndex < getSectorCount(&layer->sectors);
		sectorIndex++)
	{
		ZoneSector *sector = &layer->sectors.sectors[sectorIndex];

		sector->zoneSectorFlags = 0;

		for (s32 relY=0; relY < sector->bounds.h; relY++)
		{
			for (s32 relX=0; relX < sector->bounds.w; relX++)
			{
				ZoneType zone = getZoneAt(city, sector->bounds.x + relX, sector->bounds.y + relY);
				if (zone == Zone_None) continue;

				bool isFilled = buildingExistsAtPosition(city, sector->bounds.x + relX, sector->bounds.y + relY);
				switch (zone)
				{
					case Zone_Residential:
					{
						sector->zoneSectorFlags |= ZoneSector_HasResZones;
						if (!isFilled) sector->zoneSectorFlags |= ZoneSector_HasEmptyResZones;
					} break;

					case Zone_Commercial:
					{
						sector->zoneSectorFlags |= ZoneSector_HasComZones;
						if (!isFilled) sector->zoneSectorFlags |= ZoneSector_HasEmptyComZones;
					} break;

					case Zone_Industrial:
					{
						sector->zoneSectorFlags |= ZoneSector_HasIndZones;
						if (!isFilled) sector->zoneSectorFlags |= ZoneSector_HasEmptyIndZones;
					} break;
				}
			}
		}

		if (sector->zoneSectorFlags & ZoneSector_HasResZones)
		{
			setBit(&layer->sectorsWithZones[Zone_Residential], sectorIndex, true);
		}
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyResZones)
		{
			setBit(&layer->sectorsWithEmptyZones[Zone_Residential], sectorIndex, true);
		}
		if (sector->zoneSectorFlags & ZoneSector_HasComZones)
		{
			setBit(&layer->sectorsWithZones[Zone_Commercial], sectorIndex, true);
		}
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyComZones)
		{
			setBit(&layer->sectorsWithEmptyZones[Zone_Commercial], sectorIndex, true);
		}
		if (sector->zoneSectorFlags & ZoneSector_HasIndZones)
		{
			setBit(&layer->sectorsWithZones[Zone_Industrial], sectorIndex, true);
		}
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyIndZones)
		{
			setBit(&layer->sectorsWithEmptyZones[Zone_Industrial], sectorIndex, true);
		}
	}

	calculateDemand(city, layer);
	growSomeZoneBuildings(city);
}

void calculateDemand(City *city, ZoneLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);
	
	// Ratio of residents to job should be roughly 3 : 1

	// TODO: We want to consider AVAILABLE jobs/residents, not TOTAL ones.
	// TODO: This is a generally terrible calculation!

	// Residential
	layer->demand[Zone_Residential] = (city->totalJobs * 3) - city->totalResidents + 50;

	// Commercial
	layer->demand[Zone_Commercial] = (city->totalResidents / 6) + 10;

	// Industrial
	layer->demand[Zone_Industrial] = (city->totalResidents / 3) - city->totalJobs + 100;
}

bool isZoneAcceptable(City *city, ZoneType zoneType, s32 x, s32 y)
{
	DEBUG_FUNCTION();
	
	ZoneDef def = zoneDefs[zoneType];

	bool isAcceptable = true;

	if (getZoneAt(city, x, y) != zoneType)
	{
		isAcceptable = false;
	}
	else if (buildingExistsAtPosition(city, x, y))
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
	DEBUG_FUNCTION_T(DCDT_Simulation);

	ZoneLayer *layer = &city->zoneLayer;
	Random *random = city->gameRandom;

	for (ZoneType zoneType = FirstZoneType; zoneType < ZoneCount; zoneType = (ZoneType)(zoneType + 1))
	{
		if (layer->demand[zoneType] > 0)
		{
			s32 remainingDemand = layer->demand[zoneType];
			s32 minimumDemand = layer->demand[zoneType] / 20; // Stop when we're below 20% of the original demand

			s32 maxRBuildingDim = getMaxBuildingSize(zoneType);

			// TODO: Stop when we've grown X buildings, because we don't want to grow a whole city at once!
			while ((layer->sectorsWithEmptyZones[zoneType].setBitCount > 0) && (remainingDemand > minimumDemand))
			{
				// Find a valid res zone
				// TODO: Better selection than just a random one @Desirability
				bool foundAZone = false;
				s32 randomSectorOffset = randomNext(random);
				s32 randomXOffset = randomNext(random);
				s32 randomYOffset = randomNext(random);
				V2I zonePos = {};
				{
					DEBUG_BLOCK_T("growSomeZoneBuildings - find a valid zone", DCDT_Simulation);
					Array<s32> validSectors = getSetBitIndices(&layer->sectorsWithEmptyZones[zoneType]);
					for (s32 i = 0; i < validSectors.count; i++)
					{
						s32 sectorIndex = validSectors[(i + randomSectorOffset) % validSectors.count];
						ZoneSector *sector = &layer->sectors.sectors[sectorIndex];

						for (s32 relY=0;
							!foundAZone && relY < sector->bounds.h;
							relY++)
						{
							for (s32 relX=0;
								!foundAZone && relX < sector->bounds.w;
								relX++)
							{
								s32 tileX = (relX + randomXOffset) % sector->bounds.w;
								s32 tileY = (relY + randomYOffset) % sector->bounds.h;
								s32 x = sector->bounds.x + tileX;
								s32 y = sector->bounds.y + tileY;

								if (isZoneAcceptable(city, zoneType, x, y))
								{
									zonePos = v2i(x, y);
									foundAZone = true;
								}
							}
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
					DEBUG_BLOCK_T("growSomeZoneBuildings - expand rect", DCDT_Simulation);
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
								if (!isZoneAcceptable(city, zoneType, x, y))
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
								if (!isZoneAcceptable(city, zoneType, x, y))
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
						// TODO: Be smarter about trying different directions, so that we get larger
						// areas more often.
						if (!canExpand) break;

						// No need to grow bigger than the largest possible building!
						// (Unless at some point we do "batches" of buildings like SC4 does)
						if (zoneFootprint.w >= maxRBuildingDim && zoneFootprint.h >= maxRBuildingDim) break;

						tryX = !tryX; // Alternate x and y
						positive = randomBool(random); // random direction to expand in
					}
				}

				// Pick a building def that fits the space and is not more than 10% more than the remaining demand
				s32 maxPopulation = (s32) ((f32)remainingDemand * 1.1f);
				BuildingDef *buildingDef = findGrowableBuildingDef(random, zoneType, zoneFootprint.size, 1, maxPopulation);

				if (buildingDef)
				{
					// Place it!
					// TODO: This picks a random spot within the zoneFootprint; we should probably pick the most desirable part? @Desirability
					Rect2I footprint = randomlyPlaceRectangle(random, buildingDef->size, zoneFootprint);

					Building *building = addBuilding(city, buildingDef, footprint);
					city->totalResidents += building->currentResidents;
					city->totalJobs += building->currentJobs;
					updateBuildingTexture(city, building, buildingDef);

					remainingDemand -= (building->currentResidents + building->currentJobs);
				}
				else
				{
					// We failed to find a building def, so we should probably stop trying to build things!
					// Otherwise, we'll get stuck in an infinite loop
					break;
				}
			}
		}
	}
}
