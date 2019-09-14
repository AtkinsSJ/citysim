#pragma once

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = areaOf(city->bounds);
	zoneLayer->tileZone = allocateMultiple<ZoneType>(gameArena, cityArea);
	zoneLayer->nextSectorUpdateIndex = 0;
	zoneLayer->sectorsToUpdatePerTick = 8;

	initSectorGrid(&zoneLayer->sectors, gameArena, city->bounds.w, city->bounds.h, 16);
	s32 sectorCount = getSectorCount(&zoneLayer->sectors);

	// NB: Element 0 is empty because tracking spots with no zone is not useful
	for (s32 zoneType = FirstZoneType; zoneType < ZoneCount; zoneType++)
	{
		initBitArray(&zoneLayer->sectorsWithZones[zoneType],      gameArena, sectorCount);
		initBitArray(&zoneLayer->sectorsWithEmptyZones[zoneType], gameArena, sectorCount);

		zoneLayer->tileDesirability[zoneType] = allocateMultiple<u8>(gameArena, cityArea);

		zoneLayer->mostDesirableSectors[zoneType] = allocateArray<s32>(gameArena, sectorCount);
		for (s32 sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++)
		{
			// To start with, we just fill the array 0-to-N because we don't know what's the most desirable.
			zoneLayer->mostDesirableSectors[zoneType][sectorIndex] = sectorIndex;
		}
	}

	for (s32 sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++)
	{
		ZoneSector *sector = &zoneLayer->sectors.sectors[sectorIndex];

		sector->zoneSectorFlags = 0;
	}
}

inline ZoneDef getZoneDef(s32 zoneType)
{
	ASSERT(zoneType >= 0 && zoneType < ZoneCount);
	return zoneDefs[zoneType];
}

inline ZoneType getZoneAt(City *city, s32 x, s32 y)
{
	ZoneType result = Zone_None;
	if (tileExists(city, x, y))
	{
		result = getTileValue(city, city->zoneLayer.tileZone, x, y);
	}

	return result;
}

inline s32 calculateZoneCost(CanZoneQuery *query)
{
	return query->zoneableTilesCount * query->zoneDef->costPerTile;
}

void drawDesirabilityDataLayer(City *city, Rect2I visibleTileBounds, ZoneType zoneType)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	ASSERT(zoneType >= FirstZoneType && zoneType < ZoneCount);

	// @Copypasta drawLandValueDataLayer()
	// The TODO: @Speed: from that applies! This is very indirect.

	u8 *data = copyRegion(city->zoneLayer.tileDesirability[zoneType], city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	static Array<V4> palette = makeGradientPalette(&globalAppState.gameState->gameArena, color255(255, 0, 0, 128), color255(0, 255, 0, 128), 256);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette.count, palette.items);
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
			else if (buildingExistsAt(city, x, y))
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

void drawZones(City *city, Rect2I visibleTileBounds, s8 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	s32 zoneType = -1;
	V4 zoneColor = {};

	// TODO: @Speed: areaOf() is a poor heuristic! It's safely >= the actual value, but it would be better to
	// actually see how many there are. Though that'd be a double-iteration, unless we keep a cached count.
	DrawRectsGroup *group = beginRectsGroupUntextured(&renderer->worldBuffer, shaderID, areaOf(visibleTileBounds));

	for (s32 y = visibleTileBounds.y;
		y < visibleTileBounds.y + visibleTileBounds.h;
		y++)
	{
		spriteBounds.y = (f32) y;
		for (s32 x = visibleTileBounds.x;
			x < visibleTileBounds.x + visibleTileBounds.w;
			x++)
		{
			ZoneType zone = getZoneAt(city, x, y);
			if (zone != Zone_None)
			{
				if (zone != zoneType)
				{
					zoneType = zone;
					zoneColor = getZoneDef(zoneType).color;
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
			if ((getTileValue(city, zoneLayer->tileZone, x, y) != zoneType)
			// Terrain must be buildable
			// @Speed: URGH this terrain lookup for every tile is nasty!
			&& (get(&terrainDefs, getTerrainAt(city, x, y)->type)->canBuildOn)
			&& (!buildingExistsAt(city, x, y)))
			{
				setTile(city, zoneLayer->tileZone, x, y, zoneType);
			}
		}
	}

	// TODO: mark the affected zone sectors as dirty

	// Zones carry power!
	markAreaDirty(city, area);
}

void markZonesAsEmpty(City * /*city*/, Rect2I /*footprint*/)
{
	DEBUG_FUNCTION();
	
	// This now doesn't do anything but I'm keeping it because we probably want dirty rects/local updates later.
}

void updateZoneLayer(City *city, ZoneLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	for (s32 i = 0; i < layer->sectorsToUpdatePerTick; i++)
	{
		s32 sectorIndex = layer->nextSectorUpdateIndex;
		ZoneSector *sector = &layer->sectors[sectorIndex];

		// What zones does it contain?
		{
			DEBUG_BLOCK_T("updateZoneLayer: zone contents", DCDT_Simulation);

			for (s32 zoneType = FirstZoneType; zoneType < ZoneCount; zoneType++)
			{
				unsetBit(&layer->sectorsWithZones     [zoneType], sectorIndex);
				unsetBit(&layer->sectorsWithEmptyZones[zoneType], sectorIndex);
			}

			sector->zoneSectorFlags = 0;
			
			for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++)
			{
				for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++)
				{
					ZoneType zone = getZoneAt(city, x, y);
					if (zone == Zone_None) continue;

					bool isFilled = buildingExistsAt(city, x, y);
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
				setBit(&layer->sectorsWithZones[Zone_Residential], sectorIndex);
			}
			if (sector->zoneSectorFlags & ZoneSector_HasEmptyResZones)
			{
				setBit(&layer->sectorsWithEmptyZones[Zone_Residential], sectorIndex);
			}
			if (sector->zoneSectorFlags & ZoneSector_HasComZones)
			{
				setBit(&layer->sectorsWithZones[Zone_Commercial], sectorIndex);
			}
			if (sector->zoneSectorFlags & ZoneSector_HasEmptyComZones)
			{
				setBit(&layer->sectorsWithEmptyZones[Zone_Commercial], sectorIndex);
			}
			if (sector->zoneSectorFlags & ZoneSector_HasIndZones)
			{
				setBit(&layer->sectorsWithZones[Zone_Industrial], sectorIndex);
			}
			if (sector->zoneSectorFlags & ZoneSector_HasEmptyIndZones)
			{
				setBit(&layer->sectorsWithEmptyZones[Zone_Industrial], sectorIndex);
			}
		}

		// What's the desirability?
		{
			DEBUG_BLOCK_T("updateZoneLayer: desirability", DCDT_Simulation);

			f32 totalResDesirability = 0.0f;
			f32 totalComDesirability = 0.0f;
			f32 totalIndDesirability = 0.0f;

			for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++)
			{
				for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++)
				{
					// Residential
					{
						// Land value = good
						f32 desirability = getLandValuePercentAt(city, x, y);

						// pollution = bad
						desirability -= getPollutionPercentAt(city, x, y) * 0.4f;

						desirability = clamp01(desirability);
						setTile(city, layer->tileDesirability[Zone_Residential], x, y, (u8)(desirability * 255.0f));

						totalResDesirability += desirability;
					}

					// Commercial
					{
						// Land value = good
						f32 desirability = getLandValuePercentAt(city, x, y);

						// pollution = bad
						desirability -= getPollutionPercentAt(city, x, y) * 0.3f;

						desirability = clamp01(desirability);
						setTile(city, layer->tileDesirability[Zone_Commercial], x, y, (u8)(desirability * 255.0f));

						totalComDesirability += desirability;
					}

					// Industrial
					{
						// Lower land value is better
						f32 desirability = 1.0f - getLandValuePercentAt(city, x, y);

						// pollution = slightly bad
						desirability -= getPollutionPercentAt(city, x, y) * 0.15f;

						desirability = clamp01(desirability);
						setTile(city, layer->tileDesirability[Zone_Industrial], x, y, (u8)(desirability * 255.0f));

						totalIndDesirability += desirability;
					}
				}
			}

			s32 sectorArea = areaOf(sector->bounds);
			f32 invSectorArea = 1.0f / (f32)sectorArea;
			sector->averageDesirability[Zone_Residential] = totalResDesirability * invSectorArea;
			sector->averageDesirability[Zone_Commercial]  = totalComDesirability * invSectorArea;
			sector->averageDesirability[Zone_Industrial]  = totalIndDesirability * invSectorArea;
		}

		layer->nextSectorUpdateIndex = (layer->nextSectorUpdateIndex + 1) % getSectorCount(&layer->sectors);
	}

	// Sort the mostDesirableSectors array
	for (s32 zoneType = FirstZoneType; zoneType < ZoneCount; zoneType++)
	{
		sortArray(&layer->mostDesirableSectors[zoneType], [&](s32 sectorIndexA, s32 sectorIndexB){
			return layer->sectors[sectorIndexA].averageDesirability[zoneType] > layer->sectors[sectorIndexB].averageDesirability[zoneType];
		});

		// TEST
		#if 0
		logInfo("Most desirable sectors ({0})", {zoneDefs[zoneType].name});
		for (s32 position = 0; position < getSectorCount(&layer->sectors); position++)
		{
			s32 sectorIndex = layer->mostDesirableSectors[zoneType][position];
			f32 averageDesirability = layer->sectors[sectorIndex].averageDesirability[zoneType];
			logInfo("#{0}: sector {1}, desirability {2}", {formatInt(position), formatInt(sectorIndex), formatFloat(averageDesirability, 3)});
		}
		#endif
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

	s32 totalResidents = getTotalResidents(city);
	s32 totalJobs = getTotalJobs(city);

	// Residential
	layer->demand[Zone_Residential] = (totalJobs * 3) - totalResidents + 100;

	s32 totalJobsNeeded = (totalResidents / 3);

	// Commercial
	layer->demand[Zone_Commercial] = floor_s32(totalJobsNeeded * 0.2f) - layer->population[Zone_Commercial] + 20;

	// Industrial
	layer->demand[Zone_Industrial] = floor_s32(totalJobsNeeded * 0.8f) - layer->population[Zone_Industrial] + 50;
}

bool isZoneAcceptable(City *city, ZoneType zoneType, s32 x, s32 y)
{
	DEBUG_FUNCTION();
	
	ZoneDef def = getZoneDef(zoneType);

	bool isAcceptable = true;

	if (getZoneAt(city, x, y) != zoneType)
	{
		isAcceptable = false;
	}
	else if (buildingExistsAt(city, x, y))
	{
		isAcceptable = false;
	}
	else if (getDistanceToTransport(city, x, y, Transport_Road) > def.maximumDistanceToRoad)
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
			s32 remainingBuildingCount = 8; // How many buildings can be constructed at once TODO: Tweak this number!
			s32 remainingDemand = layer->demand[zoneType];
			s32 minimumDemand = layer->demand[zoneType] / 20; // Stop when we're below 20% of the original demand

			s32 maxRBuildingDim = getMaxBuildingSize(zoneType);

			while ((remainingBuildingCount > 0)
				&& (layer->sectorsWithEmptyZones[zoneType].setBitCount > 0)
				&& (remainingDemand > minimumDemand))
			{
				bool foundAZone = false;
				s32 randomXOffset = randomNext(random);
				s32 randomYOffset = randomNext(random);
				V2I zonePos = {};
				{
					DEBUG_BLOCK_T("growSomeZoneBuildings - find a valid zone", DCDT_Simulation);

					// Go through sectors from most desirable to least
					for (s32 position = 0; position < getSectorCount(&layer->sectors); position++)
					{
						s32 sectorIndex = layer->mostDesirableSectors[zoneType][position];

						// Skip if it doesn't have any available zones
						// TODO: We want to penalise redevelopment, but still allow it
						if (!layer->sectorsWithEmptyZones[zoneType][sectorIndex]) continue;

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
				BuildingDef *buildingDef = findRandomZoneBuilding(zoneType, random, [=](BuildingDef *it) -> bool {
					if ((it->width > zoneFootprint.w) || (it->height > zoneFootprint.h)) return false;

					if (it->growsInZone == Zone_Residential)
					{
						return (it->residents > 0) && (it->residents <= maxPopulation);
					}
					else
					{
						return (it->jobs > 0) && (it->jobs <= maxPopulation);
					}
				});

				if (buildingDef)
				{
					// Place it!
					// TODO: This picks a random spot within the zoneFootprint; we should probably pick the most desirable part? @Desirability
					Rect2I footprint = randomlyPlaceRectangle(random, buildingDef->size, zoneFootprint);

					Building *building = addBuilding(city, buildingDef, footprint);
					layer->population[zoneType] += building->currentResidents + building->currentJobs;
					updateBuildingTexture(city, building, buildingDef);

					markAreaDirty(city, footprint);

					remainingDemand -= (building->currentResidents + building->currentJobs);
					remainingBuildingCount--;
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

inline s32 getTotalResidents(City *city)
{
	return city->zoneLayer.population[Zone_Residential];
}

inline s32 getTotalJobs(City *city)
{
	return city->zoneLayer.population[Zone_Commercial] + city->zoneLayer.population[Zone_Industrial] + city->zoneLayer.population[Zone_None];
}
