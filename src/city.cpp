#pragma once

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;

	initChunkPool(&city->sectorBuildingsChunkPool,   gameArena, 32);
	initChunkPool(&city->sectorBoundariesChunkPool,  gameArena, 8);

	initSectorGrid(&city->sectors, gameArena, width, height, 16);
	for (s32 sectorIndex = 0; sectorIndex < city->sectors.count; sectorIndex++)
	{
		Sector *sector = city->sectors.sectors + sectorIndex;

		sector->terrain       = PushArray(gameArena, Terrain,         sector->bounds.w * sector->bounds.h);
		sector->tileBuilding  = PushArray(gameArena, TileBuildingRef, sector->bounds.w * sector->bounds.h);
		sector->tilePathGroup = PushArray(gameArena, s32,             sector->bounds.w * sector->bounds.h);

		initChunkedArray(&sector->buildings, &city->sectorBuildingsChunkPool);
	}

	initPowerLayer(&city->powerLayer, city, gameArena);
	initZoneLayer(&city->zoneLayer, city, gameArena);

	city->highestBuildingID = 0;
}

Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint)
{
	DEBUG_FUNCTION();

	Sector *ownerSector = getSectorAtTilePos(&city->sectors, footprint.x, footprint.y);
	
	s32 localIndex = (s32) ownerSector->buildings.count;
	Building *building = appendBlank(&ownerSector->buildings);
	building->id = ++city->highestBuildingID;
	building->typeID = def->typeID;
	building->footprint = footprint;

	// TODO: Properly calculate occupancy!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	// TODO: Optimise this per-sector!
	for (s32 y = footprint.y;
		y < footprint.y + footprint.h;
		y++)
	{
		for (s32 x = footprint.x;
			x < footprint.x + footprint.w;
			x++)
		{
			Sector *sector = getSectorAtTilePos(&city->sectors, x, y);
			s32 relX = x - sector->bounds.x;
			s32 relY = y - sector->bounds.y;

			TileBuildingRef *ref = getSectorTile(sector, sector->tileBuilding, relX, relY);
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

			Terrain *terrain = getTerrainAt(city, x, y);
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

bool canPlaceBuilding(City *city, BuildingDef *def, s32 left, s32 top)
{
	DEBUG_FUNCTION();

	// Can we afford to build this?
	if (!canAfford(city, def->buildCost))
	{
		return false;
	}

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

	// Are we in bounds?
	if (!rectInRect2I(irectXYWH(0,0, city->width, city->height), footprint))
	{
		return false;
	}

	// Check terrain is buildable and empty
	// TODO: Optimise this per-sector!
	for (s32 y=footprint.y; y<footprint.y + footprint.h; y++)
	{
		for (s32 x=footprint.x; x<footprint.x + footprint.w; x++)
		{
			Terrain *terrain = getTerrainAt(city, x, y);
			TerrainDef *terrainDef = get(&terrainDefs, terrain->type);

			if (!terrainDef->canBuildOn)
			{
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
void placeBuilding(City *city, BuildingDef *def, s32 left, s32 top)
{
	DEBUG_FUNCTION();

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

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
		placeZone(city, Zone_None, footprint);

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
}

s32 calculateBuildCost(City *city, BuildingDef *def, Rect2I area)
{
	DEBUG_FUNCTION();
	
	s32 totalCost = 0;

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			if (canPlaceBuilding(city, def, area.x + x, area.y + y))
			{
				totalCost += def->buildCost;
			}
		}
	}

	return totalCost;
}

void placeBuildingRect(City *city, BuildingDef *def, Rect2I area)
{
	DEBUG_FUNCTION();

	for (s32 y=0; y + def->height <= area.h; y += def->height)
	{
		for (s32 x=0; x + def->width <= area.w; x += def->width)
		{
			if (canPlaceBuilding(city, def, area.x + x, area.y + y))
			{
				placeBuilding(city, def, area.x + x, area.y + y);
			}
		}
	}
}

s32 calculateDemolitionCost(City *city, Rect2I area)
{
	DEBUG_FUNCTION();
	
	s32 total = 0;

	// Terrain demolition cost
	Rect2I sectorsArea = getSectorsCovered(&city->sectors, area);
	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			Sector *sector = getSector(&city->sectors, sX, sY);
			Rect2I relArea = intersectRelative(area, sector->bounds);

			for (s32 y=relArea.y;
				y < relArea.y + relArea.h;
				y++)
			{
				for (s32 x=relArea.x;
					x < relArea.x + relArea.w;
					x++)
				{
					TerrainDef *tDef = get(&terrainDefs, getSectorTile(sector, sector->terrain, x, y)->type);

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

		Rect2I sectorsArea = getSectorsCovered(&city->sectors, area);
		for (s32 sY = sectorsArea.y;
			sY < sectorsArea.y + sectorsArea.h;
			sY++)
		{
			for (s32 sX = sectorsArea.x;
				sX < sectorsArea.x + sectorsArea.w;
				sX++)
			{
				Sector *sector = getSector(&city->sectors, sX, sY);
				Rect2I relArea = intersectRelative(area, sector->bounds);

				for (s32 y=relArea.y;
					y < relArea.y + relArea.h;
					y++)
				{
					for (s32 x=relArea.x;
						x < relArea.x + relArea.w;
						x++)
					{
						Terrain *terrain = getSectorTile(sector, sector->terrain, x, y);
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
			Sector *buildingOwnerSector = getSectorAtTilePos(&city->sectors, buildingFootprint.x, buildingFootprint.y);

			TileBuildingRef *tileBuildingRef = getSectorBuildingRefAtWorldPosition(buildingOwnerSector, buildingFootprint.x, buildingFootprint.y);
			removeIndex(&buildingOwnerSector->buildings, tileBuildingRef->localIndex, false);
			building = null; // For safety, because we just deleted the Building!

			// Clear all references to this building
			Rect2I sectorsArea = getSectorsCovered(&city->sectors, buildingFootprint);
			for (s32 sY = sectorsArea.y;
				sY < sectorsArea.y + sectorsArea.h;
				sY++)
			{
				for (s32 sX = sectorsArea.x;
					sX < sectorsArea.x + sectorsArea.w;
					sX++)
				{
					Sector *sector = getSector(&city->sectors, sX, sY);
					Rect2I relArea = intersectRelative(buildingFootprint, sector->bounds);
					for (s32 relY = relArea.y;
						relY < relArea.y + relArea.h;
						relY++)
					{
						for (s32 relX = relArea.x;
							relX < relArea.x + relArea.w;
							relX++)
						{
							TileBuildingRef *tileBuilding = getSectorTile(sector, sector->tileBuilding, relX, relY);
							tileBuilding->isOccupied = false;

							// Oh wow, we're 5 loops deep. Cool? Coolcoolcoolcoolcoolcoolcoolcococococooolnodoubtnodoubtnodoubt

							// TODO: Remove this! Put recalculation in the pathlayer instead, like for power
							if (def->isPath)
							{
								// Remove from the pathing layer
								setSectorTile(sector, sector->tilePathGroup, relX, relY, 0);
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
		Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

		for (s32 sY = sectorsArea.y;
			sY < sectorsArea.y + sectorsArea.h;
			sY++)
		{
			for (s32 sX = sectorsArea.x;
				sX < sectorsArea.x + sectorsArea.w;
				sX++)
			{
				Sector *sector = getSector(&city->sectors, sX, sY);
				
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
							TileBuildingRef *ref = getSectorTile(sector, sector->tileBuilding, relX, relY);
							ref->localIndex = buildingIndex;
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

ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area, u32 flags)
{
	ChunkedArray<Building *> result = {};
	initChunkedArray(&result, globalFrameTempArena, 64);

	bool requireOriginInArea = (flags & BQF_RequireOriginInArea) != 0;

	// Expand the area to account for buildings to the left or up from it
	// (but don't do that if we only care about origins)
	s32 expansion = requireOriginInArea ? 0 : buildingCatalogue.overallMaxBuildingDim;
	Rect2I expandedArea = expand(area, expansion, 0, expansion, 0);
	Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			Sector *sector = getSector(&city->sectors, sX, sY);

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

///// SECTORS!
///// SECTORS!
///// SECTORS!
///// SECTORS!
///// SECTORS!
///// SECTORS!

template<typename SectorType>
void initSectorGrid(SectorGrid<SectorType> *grid, MemoryArena *arena, s32 cityWidth, s32 cityHeight, s32 sectorSize)
{
	grid->width  = cityWidth;
	grid->height = cityHeight;
	grid->sectorSize = sectorSize;
	grid->sectorsX = divideCeil(cityWidth, sectorSize);
	grid->sectorsY = divideCeil(cityHeight, sectorSize);
	grid->count = grid->sectorsX * grid->sectorsY;

	grid->sectors = PushArray(arena, SectorType, grid->count);

	s32 remainderWidth  = cityWidth  % sectorSize;
	s32 remainderHeight = cityHeight % sectorSize;
	for (s32 y = 0; y < grid->sectorsY; y++)
	{
		for (s32 x = 0; x < grid->sectorsX; x++)
		{
			SectorType *sector = grid->sectors + (grid->sectorsX * y) + x;

			*sector = {};
			sector->bounds = irectXYWH(x * sectorSize, y * sectorSize, sectorSize, sectorSize);

			if ((x == grid->sectorsX - 1) && remainderWidth > 0)
			{
				sector->bounds.w = remainderWidth;
			}
			if ((y == grid->sectorsY - 1) && remainderHeight > 0)
			{
				sector->bounds.h = remainderHeight;
			}
		}
	}
}

template<typename SectorType>
inline SectorType *getSector(SectorGrid<SectorType> *grid, s32 sectorX, s32 sectorY)
{
	SectorType *result = null;

	if (sectorX >= 0 && sectorX < grid->sectorsX && sectorY >= 0 && sectorY < grid->sectorsY)
	{
		result = grid->sectors + (sectorY * grid->sectorsX) + sectorX;
	}

	return result;
}

template<typename SectorType>
inline SectorType *getSectorAtTilePos(SectorGrid<SectorType> *grid, s32 x, s32 y)
{
	SectorType *result = null;

	if (x >= 0 && x < grid->width && y >= 0 && y < grid->height)
	{
		result = getSector(grid, x / grid->sectorSize, y / grid->sectorSize);
	}

	return result;
}

template<typename SectorType>
inline Rect2I getSectorsCovered(SectorGrid<SectorType> *grid, Rect2I area)
{
	area = intersect(area, irectXYWH(0, 0, grid->width, grid->height));

	Rect2I result = irectMinMax(
		area.x / grid->sectorSize,
		area.y / grid->sectorSize,

		(area.x + area.w) / grid->sectorSize,
		(area.y + area.h) / grid->sectorSize
	);

	return result;
}
