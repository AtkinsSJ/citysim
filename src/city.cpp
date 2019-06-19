#pragma once

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;

	city->sectorsX = divideCeil(width,  SECTOR_SIZE);
	city->sectorsY = divideCeil(height, SECTOR_SIZE);
	city->sectorCount = city->sectorsX * city->sectorsY;
	city->sectors = PushArray(gameArena, Sector, city->sectorCount);

	initChunkPool(&city->sectorBuildingsChunkPool,   gameArena, 32);
	initChunkPool(&city->sectorBoundariesChunkPool,  gameArena, 8);

	s32 remainderWidth  = width  % SECTOR_SIZE;
	s32 remainderHeight = height % SECTOR_SIZE;
	for (s32 y = 0; y < city->sectorsY; y++)
	{
		for (s32 x = 0; x < city->sectorsX; x++)
		{
			Sector *sector = city->sectors + (city->sectorsX * y) + x;

			*sector = {};
			sector->bounds = irectXYWH(x * SECTOR_SIZE, y * SECTOR_SIZE, SECTOR_SIZE, SECTOR_SIZE);

			if ((x == city->sectorsX - 1) && remainderWidth > 0)
			{
				sector->bounds.w = remainderWidth;
			}
			if ((y == city->sectorsY - 1) && remainderHeight > 0)
			{
				sector->bounds.h = remainderHeight;
			}

			initChunkedArray(&sector->buildings, &city->sectorBuildingsChunkPool);
		}
	}

	initPowerLayer(&city->powerLayer, city, gameArena);

	initZoneLayer(gameArena, &city->zoneLayer);

	city->highestBuildingID = 0;
}

Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint)
{
	DEBUG_FUNCTION();

	Sector *ownerSector = getSectorAtTilePos(city, footprint.x, footprint.y);
	
	s32 localIndex = (s32) ownerSector->buildings.count;
	Building *building = appendBlank(&ownerSector->buildings);
	building->id = ++city->highestBuildingID;
	building->typeID = def->typeID;
	building->footprint = footprint;

	// TODO: Properly calculate occupancy!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	// TODO: Optimise this per-sector!
	for (s32 y=0; y<footprint.h; y++)
	{
		for (s32 x=0; x<footprint.w; x++)
		{
			Sector *sector = getSectorAtTilePos(city, footprint.x+x, footprint.y+y);
			TileBuildingRef *ref = &sector->tileBuilding[footprint.y + y - sector->bounds.y][footprint.x + x - sector->bounds.x];
			ref->isOccupied = true;
			ref->originX = footprint.x;
			ref->originY = footprint.y;
			ref->isLocal = (sector == ownerSector);
			ref->localIndex = (ref->isLocal) ? localIndex : -1;
		}
	}

	return building;
}

void generatorPlaceBuilding(City *city, BuildingDef *buildingDef, s32 left, s32 top)
{
	Building *building = addBuilding(city, buildingDef, irectXYWH(left, top, buildingDef->width, buildingDef->height));
	building->spriteOffset = randomNext(&globalAppState.cosmeticRandom);
}

void generateTerrain(City *city)
{
	DEBUG_FUNCTION();
	
	u32 tGround = findTerrainTypeByName(makeString("Ground"));
	u32 tWater  = findTerrainTypeByName(makeString("Water"));

	// TODO: Replace this with a direct lookup!
	BuildingDef *bTree = findBuildingDef(makeString("Tree"));

	for (s32 y = 0; y < city->height; y++) {
		for (s32 x = 0; x < city->width; x++) {

			f32 px = (f32)x * 0.05f;
			f32 py = (f32)y * 0.05f;

			f32 perlinValue = stb_perlin_noise3(px, py, 0);

			Terrain *terrain = terrainAt(city, x, y);
			bool isGround = (perlinValue > 0.1f);
			if (isGround)
			{
				terrain->type = tWater;
			}
			else
			{
				terrain->type = tGround;

				if (stb_perlin_noise3(px, py, 1) > 0.2f)
				{
					generatorPlaceBuilding(city, bTree, x, y);
				}
			}

			terrain->spriteOffset = (s32) randomNext(&globalAppState.cosmeticRandom);
		}
	}
}

bool canAfford(City *city, s32 cost)
{
	return city->funds >= cost;
}

void spend(City *city, s32 cost)
{
	city->funds -= cost;
}

bool canPlaceBuilding(UIState *uiState, City *city, BuildingDef *def, s32 left, s32 top, bool isAttemptingToBuild = false)
{
	DEBUG_FUNCTION();

	// Can we afford to build this?
	if (!canAfford(city, def->buildCost))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, makeString("Not enough money to build this."));
		}
		return false;
	}

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

	// Are we in bounds?
	if (!rectInRect2I(irectXYWH(0,0, city->width, city->height), footprint))
	{
		if (isAttemptingToBuild)
		{
			pushUiMessage(uiState, makeString("You cannot build off the map edge."));
		}
		return false;
	}

	// Check terrain is buildable and empty
	// TODO: Optimise this per-sector!
	for (s32 y=footprint.y; y<footprint.y + footprint.h; y++)
	{
		for (s32 x=footprint.x; x<footprint.x + footprint.w; x++)
		{
			Terrain *terrain = terrainAt(city, x, y);
			TerrainDef *terrainDef = get(&terrainDefs, terrain->type);

			if (!terrainDef->canBuildOn)
			{
				if (isAttemptingToBuild)
				{
					pushUiMessage(uiState, makeString("You cannot build there."));
				}
				return false;
			}

			Building *buildingAtPos = getBuildingAtPosition(city, x, y);
			if (buildingAtPos != null)
			{
				// Check if we can combine this with the building that's already there
				if (getBuildingDef(buildingAtPos->typeID)->canBeBuiltOnID == def->typeID)
				{
					// We can!
				}
				else
				{
					return false;
				}
			}
		}
	}
	return true;
}

/**
 * Attempt to place a building. Returns whether successful.
 */
bool placeBuilding(UIState *uiState, City *city, BuildingDef *def, s32 left, s32 top, bool showBuildErrors)
{
	DEBUG_FUNCTION();
	
	if (!canPlaceBuilding(uiState, city, def, left, top, showBuildErrors))
	{
		return false;
	}

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);
	spend(city, def->buildCost);

	bool needToRecalcPaths = def->isPath;
	bool needToRecalcPower = def->carriesPower;

	Building *building = getBuildingAtPosition(city, left, top);
	if (building != null)
	{
		// Do a quick replace! We already established in canPlaceBuilding() that we match.
		// NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
		BuildingDef *oldDef = getBuildingDef(building->typeID);

		building->typeID = def->buildOverResult;
		def = getBuildingDef(def->buildOverResult);

		needToRecalcPaths = (oldDef->isPath != def->isPath);
		needToRecalcPower = (oldDef->carriesPower != def->carriesPower);

		city->totalResidents -= oldDef->residents;
		city->totalJobs -= oldDef->jobs;
	}
	else
	{
		// Remove zones
		placeZone(uiState, city, Zone_None, footprint, false);

		building = addBuilding(city, def, footprint);
	}


	// Tiles
	for (s32 y=0; y<footprint.h; y++)
	{
		for (s32 x=0; x<footprint.w; x++)
		{
			// Data layer updates
			// TODO: Local recalculation instead of completely starting over
			setPathGroup(city, footprint.x+x, footprint.y+y, def->isPath ? 1 : 0);
		}
	}

	building->currentResidents = 0;
	building->currentJobs = 0;
	city->totalResidents += def->residents;
	city->totalJobs += def->jobs;

	updateBuildingTexture(city, building, def);
	updateAdjacentBuildingTextures(city, footprint);

	if (needToRecalcPaths)
	{
		// TODO: Local recalculation instead of completely starting over
		recalculatePathingConnectivity(city);
	}

	if (needToRecalcPower)
	{
		markPowerLayerDirty(&city->powerLayer, footprint);
	}


	return true;
}

s32 calculateBuildCost(City *city, BuildingDef *def, Rect2I area)
{
	DEBUG_FUNCTION();
	
	s32 totalCost = 0;

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			if (canPlaceBuilding(null, city, def, area.x + x, area.y + y))
			{
				totalCost += def->buildCost;
			}
		}
	}

	return totalCost;
}

void placeBuildingRect(UIState *uiState, City *city, BuildingDef *def, Rect2I area)
{
	DEBUG_FUNCTION();

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			placeBuilding(uiState, city, def, area.x + x, area.y + y, false);
		}
	}
}

s32 calculateDemolitionCost(City *city, Rect2I area)
{
	DEBUG_FUNCTION();
	
	s32 total = 0;

	// Terrain demolition cost
	Rect2I sectorsArea = getSectorsCovered(city, area);
	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			Sector *sector = getSector(city, sX, sY);
			Rect2I relArea = intersectRelative(area, sector->bounds);

			for (s32 y=relArea.y;
				y < relArea.y + relArea.h;
				y++)
			{
				for (s32 x=relArea.x;
					x < relArea.x + relArea.w;
					x++)
				{
					TerrainDef *tDef = get(&terrainDefs, sector->terrain[y][x].type);

					if (tDef->canDemolish)
					{
						total += tDef->demolishCost;
					}
				}
			}
		}
	}

	// Building demolition cost
	ChunkedArray<Building *> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
	for (auto it = iterate(&buildingsToDemolish); !it.isDone; next(&it))
	{
		Building *building = getValue(it);
		total += getBuildingDef(building->typeID)->demolishCost;
	}

	return total;
}

void demolishRect(City *city, Rect2I area)
{
	DEBUG_FUNCTION();

	// NB: We assume that we've already checked we can afford this!

	// Terrain demolition
	{
		// TODO: @Cleanup Probably we want to specify what terrain something becomes per-terrain-type,
		// and maybe link it directly instead of by name?
		s32 groundTerrainType = findTerrainTypeByName(makeString("Ground"));

		Rect2I sectorsArea = getSectorsCovered(city, area);
		for (s32 sY = sectorsArea.y;
			sY < sectorsArea.y + sectorsArea.h;
			sY++)
		{
			for (s32 sX = sectorsArea.x;
				sX < sectorsArea.x + sectorsArea.w;
				sX++)
			{
				Sector *sector = getSector(city, sX, sY);
				Rect2I relArea = intersectRelative(area, sector->bounds);

				for (s32 y=relArea.y;
					y < relArea.y + relArea.h;
					y++)
				{
					for (s32 x=relArea.x;
						x < relArea.x + relArea.w;
						x++)
					{
						Terrain *terrain = &sector->terrain[y][x];
						TerrainDef *tDef = get(&terrainDefs, terrain->type);

						if (tDef->canDemolish)
						{
							terrain->type = groundTerrainType;
						}
					}
				}
			}
		}
	}

	// Building demolition
	{
		// NB: This is a little hacky... inside this loop, we remove Buildings from their Sectors.
		// Each time this happens, the buildings array in that sector gets re-ordered. This would
		// mess everything up, BUT we iterate the array in reverse, relying on the (current) fact
		// that findBuildingsOverlappingArea() returns each sector's buildings in the same order
		// they exist in that sector's buildings array. Meaning, deleting a building only affects
		// the ones AFTER it (which we've already handled!)
		// That's an implementation detail, so this could break pretty badly at some point, but
		// I think it's really unlikely that will happen?
		// Famous last words...
		// 
		// - Sam, 18/06/2019
		//
		ChunkedArray<Building *> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
		for (auto it = iterate(&buildingsToDemolish, buildingsToDemolish.count-1, false, true);
			!it.isDone;
			next(&it))
		{
			Building *building = getValue(it);
			BuildingDef *def = getBuildingDef(building->typeID);

			city->totalResidents -= def->residents;
			city->totalJobs -= def->jobs;

			Rect2I buildingFootprint = building->footprint;
			Sector *buildingOwnerSector = getSectorAtTilePos(city, buildingFootprint.x, buildingFootprint.y);

			TileBuildingRef *tileBuildingRef = getSectorBuildingRefAtWorldPosition(buildingOwnerSector, buildingFootprint.x, buildingFootprint.y);
			removeIndex(&buildingOwnerSector->buildings, tileBuildingRef->localIndex, false);
			building = null; // For safety, because we just deleted the Building!

			// Clear all references to this building
			Rect2I sectorsArea = getSectorsCovered(city, buildingFootprint);
			for (s32 sY = sectorsArea.y;
				sY < sectorsArea.y + sectorsArea.h;
				sY++)
			{
				for (s32 sX = sectorsArea.x;
					sX < sectorsArea.x + sectorsArea.w;
					sX++)
				{
					Sector *sector = getSector(city, sX, sY);
					Rect2I relArea = intersectRelative(buildingFootprint, sector->bounds);
					for (s32 relY = relArea.y;
						relY < relArea.y + relArea.h;
						relY++)
					{
						for (s32 relX = relArea.x;
							relX < relArea.x + relArea.w;
							relX++)
						{
							TileBuildingRef *tileBuilding = &sector->tileBuilding[relY][relX];
							tileBuilding->isOccupied = false;

							// Oh wow, we're 5 loops deep. Cool? Coolcoolcoolcoolcoolcoolcoolcococococooolnodoubtnodoubtnodoubt

							// TODO: Remove this! Put recalculation in the pathlayer instead, like for power
							if (def->isPath)
							{
								// Remove from the pathing layer
								sector->tilePathGroup[relY][relX] = 0;
							}
						}
					}
				}
			}

			// Need to update the filled/empty zone lists
			markZonesAsEmpty(city, buildingFootprint);
		}

		// Expand the area to account for buildings to the left or up from it
		Rect2I expandedArea = expand(area, buildingCatalogue.overallMaxBuildingDim, 0, buildingCatalogue.overallMaxBuildingDim, 0);
		Rect2I sectorsArea = getSectorsCovered(city, expandedArea);

		for (s32 sY = sectorsArea.y;
			sY < sectorsArea.y + sectorsArea.h;
			sY++)
		{
			for (s32 sX = sectorsArea.x;
				sX < sectorsArea.x + sectorsArea.w;
				sX++)
			{
				Sector *sector = getSector(city, sX, sY);
				
				// Recalculate the tile-building refs for possibly-affected sectors
				for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
				{
					Building *building = get(it);
					s32 buildingIndex = (s32) getIndex(it);

					Rect2I relArea = intersectRelative(building->footprint, sector->bounds);
					for (s32 relY=relArea.y;
						relY < relArea.y + relArea.h;
						relY++)
					{
						for (s32 relX=relArea.x;
							relX < relArea.x + relArea.w;
							relX++)
						{
							sector->tileBuilding[relY][relX].localIndex = buildingIndex;
						}
					}
				}
			}
		}

		// Any buildings that would have connected with something that just got demolished need to refresh!
		updateAdjacentBuildingTextures(city, area);

		// TODO: Local recalculation!
		recalculatePathingConnectivity(city);

		markPowerLayerDirty(&city->powerLayer, area);
	}
}

ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area)
{
	ChunkedArray<Building *> result = {};
	initChunkedArray(&result, globalFrameTempArena, 64);

	// Expand the area to account for buildings to the left or up from it
	Rect2I expandedArea = expand(area, buildingCatalogue.overallMaxBuildingDim, 0, buildingCatalogue.overallMaxBuildingDim, 0);
	Rect2I sectorsArea = getSectorsCovered(city, expandedArea);

	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			Sector *sector = getSector(city, sX, sY);

			for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
			{
				Building *building = get(it);
				if (rectsOverlap(building->footprint, area))
				{
					append(&result, building);
				}
			}
		}
	}

	return result;
}

void calculateDemand(City *city)
{
	DEBUG_FUNCTION();
	
	// Ratio of residents to job should be roughly 3 : 1

	// TODO: We want to consider AVAILABLE jobs/residents, not TOTAL ones.
	// TODO: This is a generally terrible calculation!

	// Residential
	city->residentialDemand = (city->totalJobs * 3) - city->totalResidents + 50;

	// Commercial
	city->commercialDemand = 0;

	// Industrial
	city->industrialDemand = (city->totalResidents / 3) - city->totalJobs + 30;
}

/**
 * Distance to road, counting diagonal distances as 1.
 * If nothing is found within the maxDistanceToCheck, returns a *really big number*.
 * If the tile itself is a road, just returns 0 as you'd expect.
 */
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck)
{
	DEBUG_FUNCTION();
	
	s32 result = s32Max;

	if (isPathable(city, x, y))
	{
		result = 0;
	}
	else
	{
		bool done = false;

		for (s32 distance = 1;
			 !done && distance <= maxDistanceToCheck;
			 distance++)
		{
			s32 leftX   = x - distance;
			s32 rightX  = x + distance;
			s32 bottomY = y - distance;
			s32 topY    = y + distance;

			for (s32 px = leftX; px <= rightX; px++)
			{
				if (isPathable(city, px, bottomY))
				{
					result = distance;
					done = true;
					break;
				}

				if (isPathable(city, px, topY))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (isPathable(city, leftX, py))
				{
					result = distance;
					done = true;
					break;
				}

				if (isPathable(city, rightX, py))
				{
					result = distance;
					done = true;
					break;
				}
			}
		}
	}

	return result;
}
