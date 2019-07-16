#pragma once

void initZoneLayer(ZoneLayer *zoneLayer, City *city, MemoryArena *gameArena)
{
	zoneLayer->sectorsWithResZonesCount      = 0;
	zoneLayer->sectorsWithEmptyResZonesCount = 0;
	zoneLayer->sectorsWithComZonesCount      = 0;
	zoneLayer->sectorsWithEmptyComZonesCount = 0;
	zoneLayer->sectorsWithIndZonesCount      = 0;
	zoneLayer->sectorsWithEmptyIndZonesCount = 0;

	initSectorGrid(&zoneLayer->sectors, gameArena, city->width, city->height, 16);
	for (s32 sectorIndex = 0; sectorIndex < zoneLayer->sectors.count; sectorIndex++)
	{
		ZoneSector *sector = zoneLayer->sectors.sectors + sectorIndex;

		sector->zoneSectorFlags = 0;

		sector->tileZone = allocateArray<ZoneType>(gameArena, areaOf(sector->bounds));
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
	DEBUG_FUNCTION_T(DCDT_Debugging);

	// Ignore tiles that are already this zone!
	if (getZoneAt(city, x, y) == zoneType) return false;

	TerrainDef *tDef = get(&terrainDefs, getTerrainAt(city, x, y)->type);
	if (!tDef->canBuildOn) return false;

	if (buildingExistsAtPosition(city, x, y)) return false;

	return true;
}

s32 calculateZoneCost(City *city, ZoneType zoneType, Rect2I area)
{
	DEBUG_FUNCTION_T(DCDT_Debugging);

	ZoneLayer *zoneLayer = &city->zoneLayer;

	s32 tilesToZoneCount = 0;

	// For now, you can only zone free tiles, where there are no buildings or obstructions.
	// You CAN zone over other zones though. Just, not if something has already grown in that
	// zone tile.
	// At some point we'll probably want to change this, because my least-favourite thing
	// about SC3K is that clearing a built-up zone means demolishing the buildings then quickly
	// switching to the de-zone tool before something else pops up in the free space!
	// - Sam, 13/12/2018

	Rect2I sectorsCovered = getSectorsCovered(&zoneLayer->sectors, area);
	for (s32 sY = sectorsCovered.y;
		sY < sectorsCovered.y + sectorsCovered.h;
		sY++)
	{
		for (s32 sX = sectorsCovered.x;
			sX < sectorsCovered.x + sectorsCovered.w;
			sX++)
		{
			ZoneSector *sector = getSector(&zoneLayer->sectors, sX, sY);
			Rect2I relArea = intersectRelative(sector->bounds, area);

			for (s32 relY=relArea.y;
				relY < relArea.y + relArea.h;
				relY++)
			{
				for (s32 relX=relArea.x;
					relX < relArea.x + relArea.w;
					relX++)
				{
					// Ignore tiles that are already this zone!
					if (*getSectorTile(sector, sector->tileZone, relX, relY) != zoneType)
					{
						s32 x = sector->bounds.x + relX;
						s32 y = sector->bounds.y + relY;
						
						// Tile must be buildable and empty
						TerrainDef *tDef = get(&terrainDefs, getTerrainAt(city, x, y)->type);
						if (tDef->canBuildOn && !buildingExistsAtPosition(city, x, y))
						{
							tilesToZoneCount++;
						}
					}
				}
			}
		}
	}

	s32 total = tilesToZoneCount * zoneDefs[zoneType].costPerTile;

	return total;
}

void drawZones(ZoneLayer *zoneLayer, Renderer *renderer, Rect2I visibleArea, s32 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	s32 zoneType = -1;
	V4 zoneColor = {};

	// TODO: @Speed: areaOf() is a poor heuristic! It's safely >= the actual value, but it would be better to
	// actually see how many there are. Though that'd be a double-iteration, unless we keep a cached count.
	DrawRectsGroup group = beginRectsGroup(&renderer->worldBuffer, shaderID, areaOf(visibleArea));

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

			Rect2I relArea = intersectRelative(sector->bounds, visibleArea);
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

						addUntexturedRect(&group, spriteBounds, zoneColor);
						// drawRect(&renderer->worldBuffer, spriteBounds, -900.0f, shaderID, zoneColor);
					}
				}
			}
		}
	}

	endRectsGroup(&group);
}

void placeZone(City *city, ZoneType zoneType, Rect2I area)
{
	DEBUG_FUNCTION();

	ZoneLayer *zoneLayer = &city->zoneLayer;

	Rect2I sectorsCovered = getSectorsCovered(&zoneLayer->sectors, area);
	for (s32 sY = sectorsCovered.y;
		sY < sectorsCovered.y + sectorsCovered.h;
		sY++)
	{
		for (s32 sX = sectorsCovered.x;
			sX < sectorsCovered.x + sectorsCovered.w;
			sX++)
		{
			ZoneSector *sector = getSector(&zoneLayer->sectors, sX, sY);
			Rect2I relArea = intersectRelative(sector->bounds, area);

			for (s32 relY=relArea.y;
				relY < relArea.y + relArea.h;
				relY++)
			{
				for (s32 relX=relArea.x;
					relX < relArea.x + relArea.w;
					relX++)
				{
					// Ignore tiles that are already this zone!
					if (*getSectorTile(sector, sector->tileZone, relX, relY) != zoneType)
					{
						s32 x = sector->bounds.x + relX;
						s32 y = sector->bounds.y + relY;
						
						// Tile must be buildable and empty
						TerrainDef *tDef = get(&terrainDefs, getTerrainAt(city, x, y)->type);
						if (tDef->canBuildOn && !buildingExistsAtPosition(city, x, y))
						{
							setSectorTile(sector, sector->tileZone, relX, relY, zoneType);
						}
					}
				}
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
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	layer->sectorsWithResZonesCount      = 0;
	layer->sectorsWithEmptyResZonesCount = 0;
	layer->sectorsWithComZonesCount      = 0;
	layer->sectorsWithEmptyComZonesCount = 0;
	layer->sectorsWithIndZonesCount      = 0;
	layer->sectorsWithEmptyIndZonesCount = 0;

	for (s32 sectorIndex = 0;
		sectorIndex < layer->sectors.count;
		sectorIndex++)
	{
		ZoneSector *sector = layer->sectors.sectors + sectorIndex;

		sector->zoneSectorFlags = 0;

		for (s32 relY=0; relY < sector->bounds.h; relY++)
		{
			for (s32 relX=0; relX < sector->bounds.w; relX++)
			{
				ZoneType zone = *getSectorTile(sector, sector->tileZone, relX, relY);
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

		if (sector->zoneSectorFlags & ZoneSector_HasResZones)       layer->sectorsWithResZonesCount++;
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyResZones)  layer->sectorsWithEmptyResZonesCount++;
		if (sector->zoneSectorFlags & ZoneSector_HasComZones)       layer->sectorsWithComZonesCount++;
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyComZones)  layer->sectorsWithEmptyComZonesCount++;
		if (sector->zoneSectorFlags & ZoneSector_HasIndZones)       layer->sectorsWithIndZonesCount++;
		if (sector->zoneSectorFlags & ZoneSector_HasEmptyIndZones)  layer->sectorsWithEmptyIndZonesCount++;
	}
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
	DEBUG_FUNCTION();

	ZoneLayer *layer = &city->zoneLayer;
	Random *random = city->gameRandom;

	// Residential
	if (city->residentialDemand > 0)
	{
		s32 remainingDemand = city->residentialDemand;
		s32 minimumDemand = city->residentialDemand / 20; // Stop when we're below 20% of the original demand

		s32 maxRBuildingDim = buildingCatalogue.maxRBuildingDim;

		while ((layer->sectorsWithEmptyResZonesCount > 0) && (remainingDemand > minimumDemand))
		{
			// Find a valid res zone
			// TODO: Better selection than just a random one
			bool foundAZone = false;
			s32 randomSectorOffset = randomNext(random);
			s32 randomXOffset = randomNext(random);
			s32 randomYOffset = randomNext(random);
			V2I zonePos = {};
			{
				DEBUG_BLOCK("growSomeZoneBuildings - find a valid zone");
				for (s32 sectorIndex = 0;
					!foundAZone && sectorIndex < layer->sectors.count;
					sectorIndex++)
				{
					ZoneSector *sector = layer->sectors.sectors + ((sectorIndex + randomSectorOffset) % layer->sectors.count);

					if (sector->zoneSectorFlags & ZoneSector_HasEmptyResZones)
					{
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
								// ZoneType zone = *getSectorTile(sector, sector->tileZone, tileX, tileY);
								if (isZoneAcceptable(city, Zone_Residential, x, y))
								{
									zonePos = v2i(x, y);
									foundAZone = true;
								}
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
				Rect2I footprint = irectPosSize(zoneFootprint.pos, buildingDef->size);

				Building *building = addBuilding(city, buildingDef, footprint);
				city->totalResidents += building->currentResidents;
				city->totalJobs += building->currentJobs;
				updateBuildingTexture(city, building, buildingDef);

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
