#pragma once

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->bounds = irectXYWH(0, 0, width, height);

	s32 cityArea = width * height;

	city->tileBuildingIndex = allocateMultiple<s32>    (gameArena, cityArea);
	initChunkPool(&city->sectorBuildingsChunkPool,  gameArena, 128);
	initChunkPool(&city->sectorBoundariesChunkPool, gameArena,   8);
	initChunkPool(&city->buildingRefsChunkPool,     gameArena, 128);

	initSectorGrid(&city->sectors, gameArena, width, height, 16, 8);
	for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&city->sectors); sectorIndex++)
	{
		CitySector *sector = &city->sectors.sectors[sectorIndex];
		initChunkedArray(&sector->ownedBuildings, &city->sectorBuildingsChunkPool);
	}

	initOccupancyArray(&city->buildings, gameArena, 1024);
	append(&city->buildings);

	initCrimeLayer    (&city->crimeLayer,     city, gameArena);
	initFireLayer     (&city->fireLayer,      city, gameArena);
	initHealthLayer   (&city->healthLayer,    city, gameArena);
	initLandValueLayer(&city->landValueLayer, city, gameArena);
	initPollutionLayer(&city->pollutionLayer, city, gameArena);
	initPowerLayer    (&city->powerLayer,     city, gameArena);
	initTerrainLayer  (&city->terrainLayer,   city, gameArena);
	initTransportLayer(&city->transportLayer, city, gameArena);
	initZoneLayer     (&city->zoneLayer,      city, gameArena);

	city->highestBuildingID = 0;

	markAreaDirty(city, city->bounds);
}

Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint)
{
	DEBUG_FUNCTION();

	auto buildingSlot = append(&city->buildings);
	s32 buildingIndex = buildingSlot.index;
	Building *building = buildingSlot.item;
	building->id = ++city->highestBuildingID;
	building->typeID = def->typeID;
	building->footprint = footprint;
	initFlags(&building->problems, BuildingProblemCount);

	CitySector *ownerSector = getSectorAtTilePos(&city->sectors, footprint.x, footprint.y);
	append(&ownerSector->ownedBuildings, building);

	// TODO: Properly calculate occupancy!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	for (s32 y = footprint.y;
		y < footprint.y + footprint.h;
		y++)
	{
		for (s32 x = footprint.x;
			x < footprint.x + footprint.w;
			x++)
		{
			setTile(city, city->tileBuildingIndex, x, y, buildingIndex);
		}
	}

	if (hasEffect(&def->fireProtection))
	{
		registerFireProtectionBuilding(&city->fireLayer, building);
	}

	if (hasEffect(&def->healthEffect))
	{
		registerHealthBuilding(&city->healthLayer, building);
	}

	if (hasEffect(&def->policeEffect) || (def->jailCapacity > 0))
	{
		registerPoliceBuilding(&city->crimeLayer, building);
	}

	if (def->power > 0)
	{
		registerPowerBuilding(&city->powerLayer, building);
	}

	return building;
}

void markAreaDirty(City *city, Rect2I bounds)
{
	markHealthLayerDirty   (&city->healthLayer, bounds);
	markLandValueLayerDirty(&city->landValueLayer, bounds);
	markPollutionLayerDirty(&city->pollutionLayer, bounds);
	markPowerLayerDirty    (&city->powerLayer, bounds);
	markTransportLayerDirty(&city->transportLayer, bounds);
}

inline bool tileExists(City *city, s32 x, s32 y)
{
	return (x >= 0) && (x < city->bounds.w)
		&& (y >= 0) && (y < city->bounds.h);
}

template<typename T>
inline T *getTile(City *city, T *tiles, s32 x, s32 y)
{
	return tiles + ((y * city->bounds.w) + x);
}

template<typename T>
inline T getTileValue(City *city, T *tiles, s32 x, s32 y)
{
	return tiles[((y * city->bounds.w) + x)];
}

template<typename T>
inline T getTileValueIfExists(City *city, T *tiles, s32 x, s32 y, T defaultValue)
{
	if (tileExists(city, x, y))
	{
		return getTileValue(city, tiles, x, y);
	}
	else
	{
		return defaultValue;
	}
}

template<typename T>
inline void setTile(City *city, T *tiles, s32 x, s32 y, T value)
{
	tiles[(y * city->bounds.w) + x] = value;
}

inline bool canAfford(City *city, s32 cost)
{
	return city->funds >= cost;
}

inline void spend(City *city, s32 cost)
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
	if (!contains(irectXYWH(0,0, city->bounds.w, city->bounds.h), footprint))
	{
		return false;
	}

	// Check terrain is buildable and empty
	// TODO: Optimise this per-sector!
	for (s32 y=footprint.y; y<footprint.y + footprint.h; y++)
	{
		for (s32 x=footprint.x; x<footprint.x + footprint.w; x++)
		{
			TerrainDef *terrainDef = getTerrainAt(city, x, y);

			if (!terrainDef->canBuildOn)
			{
				return false;
			}

			Building *buildingAtPos = getBuildingAt(city, x, y);
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

void placeBuilding(City *city, BuildingDef *def, s32 left, s32 top, bool markAreasDirty)
{
	DEBUG_FUNCTION();

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

	bool needToRecalcTransport = !isEmpty(&def->transportTypes);
	bool needToRecalcPower = (def->flags & Building_CarriesPower);

	Building *building = getBuildingAt(city, left, top);
	if (building != null)
	{
		// Do a quick replace! We already established in canPlaceBuilding() that we match.
		// NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
		BuildingDef *oldDef = getBuildingDef(building->typeID);

		building->typeID = def->buildOverResult;
		def = getBuildingDef(def->buildOverResult);

		needToRecalcTransport = (oldDef->transportTypes != def->transportTypes);
		needToRecalcPower = ((oldDef->flags & Building_CarriesPower) != (def->flags & Building_CarriesPower));

		city->zoneLayer.population[oldDef->growsInZone] -= building->currentResidents + building->currentJobs;
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
			// This is a @Hack! If we don't do this, then the building texture linking doesn't work,
			// because it reads the transport info for the tile instead of looking up the tile's Building.
			// The TransportLayer overwrites this later when it does a refresh of the area, but that will
			// happen at an unknown point in the future after this function ends.
			// So, we stuff temporary data in here so we can read it in ~10 lines' time!
			// IDK, the texture-linking stuff really needs a rethink.
			// - Sam, 28/08/2019
			addTransportToTile(city, footprint.x+x, footprint.y+y, def->transportTypes);
		}
	}

	// TODO: Calculate residents/jobs properly!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;

	updateBuildingTexture(city, building, def);
	updateAdjacentBuildingTextures(city, footprint);

	if (markAreasDirty)
	{
		markAreaDirty(city, footprint);
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
				placeBuilding(city, def, area.x + x, area.y + y, false);
			}
		}
	}

	markAreaDirty(city, area);
}

s32 calculateDemolitionCost(City *city, Rect2I area)
{
	DEBUG_FUNCTION();
	
	s32 total = 0;

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

	// Building demolition
	ChunkedArray<Building *> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
	for (auto it = iterate(&buildingsToDemolish, buildingsToDemolish.count-1, false, true);
		!it.isDone;
		next(&it))
	{
		Building *building = getValue(it);
		BuildingDef *def = getBuildingDef(building->typeID);

		city->zoneLayer.population[def->growsInZone] -= building->currentResidents + building->currentJobs;

		Rect2I buildingFootprint = building->footprint;

		// Clean up other references
		if (hasEffect(&def->fireProtection))
		{
			unregisterFireProtectionBuilding(&city->fireLayer, building);
		}

		if (hasEffect(&def->healthEffect))
		{
			unregisterHealthBuilding(&city->healthLayer, building);
		}

		if (hasEffect(&def->policeEffect) || (def->jailCapacity > 0))
		{
			unregisterPoliceBuilding(&city->crimeLayer, building);
		}

		if (def->power > 0)
		{
			unregisterPowerBuilding(&city->powerLayer, building);
		}

		building->id = 0;
		building->typeID = -1;

		s32 buildingIndex = getTileValue(city, city->tileBuildingIndex, buildingFootprint.x, buildingFootprint.y);
		removeIndex(&city->buildings, buildingIndex);

		building = null; // For safety, because we just deleted the Building!

		for (s32 y = buildingFootprint.y;
			y < buildingFootprint.y + buildingFootprint.h;
			y++)
		{
			for (s32 x = buildingFootprint.x;
				x < buildingFootprint.x + buildingFootprint.w;
				x++)
			{
				setTile(city, city->tileBuildingIndex, x, y, 0);
			}
		}

		// Only need to add the footprint as a separate rect if it's not inside the area!
		if (!contains(area, buildingFootprint))
		{
			markZonesAsEmpty(city, buildingFootprint);
			markAreaDirty(city, buildingFootprint);
		}
	}

	// Expand the area to account for buildings to the left or up from it
	Rect2I expandedArea = expand(area, buildingCatalogue.overallMaxBuildingDim, 0, 0, buildingCatalogue.overallMaxBuildingDim);
	Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			CitySector *sector = getSector(&city->sectors, sX, sY);
			
			// Rebuild the ownedBuildings array
			clear(&sector->ownedBuildings);

			for (s32 y = sector->bounds.y;
				y < sector->bounds.y + sector->bounds.h;
				y++)
			{
				for (s32 x = sector->bounds.x;
					x < sector->bounds.x + sector->bounds.w;
					x++)
				{
					Building *b = getBuildingAt(city, x, y);
					if (b != null)
					{
						if (b->footprint.x == x && b->footprint.y == y)
						{
							append(&sector->ownedBuildings, b);
						}
					}
				}
			}
		}
	}

	// Mark area as changed
	markZonesAsEmpty(city, area);
	markAreaDirty(city, area);

	// Any buildings that would have connected with something that just got demolished need to refresh!
	updateAdjacentBuildingTextures(city, area);
}

ChunkedArray<Building *> findBuildingsOverlappingArea(City *city, Rect2I area, u32 flags)
{
	DEBUG_FUNCTION();
	
	ChunkedArray<Building *> result = {};
	initChunkedArray(&result, tempArena, 64);

	bool requireOriginInArea = (flags & BQF_RequireOriginInArea) != 0;

	// Expand the area to account for buildings to the left or up from it
	// (but don't do that if we only care about origins)
	s32 expansion = requireOriginInArea ? 0 : buildingCatalogue.overallMaxBuildingDim;
	Rect2I expandedArea = expand(area, expansion, 0, 0, expansion);
	Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

	for (s32 sY = sectorsArea.y;
		sY < sectorsArea.y + sectorsArea.h;
		sY++)
	{
		for (s32 sX = sectorsArea.x;
			sX < sectorsArea.x + sectorsArea.w;
			sX++)
		{
			CitySector *sector = getSector(&city->sectors, sX, sY);

			for (auto it = iterate(&sector->ownedBuildings); !it.isDone; next(&it))
			{
				Building *building = getValue(it);
				if (overlaps(building->footprint, area))
				{
					append(&result, building);
				}
			}
		}
	}

	return result;
}

void drawCity(City *city, Rect2I visibleTileBounds, Rect2I demolitionRect)
{
	drawTerrain(city, visibleTileBounds, renderer->shaderIds.pixelArt);

	drawZones(city, visibleTileBounds, renderer->shaderIds.untextured);

	drawBuildings(city, visibleTileBounds, renderer->shaderIds.pixelArt, demolitionRect);

	// Draw effects
	drawFires(city, visibleTileBounds);

	// Draw sectors
	// NB: this is really hacky debug code
	if (false)
	{
		Rect2I visibleSectors = getSectorsCovered(&city->sectors, visibleTileBounds);
		DrawRectsGroup *group = beginRectsGroupUntextured(&renderer->worldOverlayBuffer, renderer->shaderIds.untextured, areaOf(visibleSectors));
		V4 sectorColor = color255(255, 255, 255, 40);
		for (s32 sy = visibleSectors.y;
			sy < visibleSectors.y + visibleSectors.h;
			sy++)
		{
			for (s32 sx = visibleSectors.x;
				sx < visibleSectors.x + visibleSectors.w;
				sx++)
			{
				if ((sx + sy) % 2)
				{
					CitySector *sector = getSector(&city->sectors, sx, sy);
					addUntexturedRect(group, rect2(sector->bounds), sectorColor);
				}
			}
		}
		endRectsGroup(group);
	}
}

inline bool buildingExistsAt(City *city, s32 x, s32 y)
{
	bool result = getTileValue(city, city->tileBuildingIndex, x, y) > 0;

	return result;
}

Building* getBuildingAt(City *city, s32 x, s32 y)
{
	Building *result = null;

	if (tileExists(city, x, y))
	{
		u32 buildingID = getTileValue(city, city->tileBuildingIndex, x, y);
		if (buildingID > 0)
		{
			result = get(&city->buildings, buildingID);
		}
	}

	return result;
}

void drawBuildings(City *city, Rect2I visibleTileBounds, s8 shaderID, Rect2I demolitionRect)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	V4 drawColorNormal = makeWhite();
	V4 drawColorDemolish = color255(255,128,128,255);
	V4 drawColorNoPower = color255(32,32,64,255);

	s32 typeID = -1;
	SpriteGroup *sprites = null;

	bool isDemolitionHappening = areaOf(demolitionRect) > 0;

	//
	// TODO: Once buildings have "height" that extends above their footprint, we'll need to know
	// the maximum height, and go a corresponding number of sectors down to ensure they're drawn.
	//
	// - Sam, 17/06/2019
	//

	// TODO: @Speed: Maybe do the iteration on sectors directly, instead of producing this
	// array and then iterating it? Or some other smarter way.
	ChunkedArray<Building *> visibleBuildings = findBuildingsOverlappingArea(city, visibleTileBounds);

	if (visibleBuildings.count == 0) return;

	//
	// NB: We don't know how many of the buildings will be in each batch, because a batch can
	// only have a single texture. So we track how many buildings HAVEN'T been drawn yet, and
	// each time we start a batch we set it to that size. That way it's always large enough,
	// without being TOO excessive.
	// Theoretically later we could smush things into fewer textures, and that would speed
	// this up, but I think we'll always need the "is the texture different?" check in the loop.
	//
	// - Sam, 16/07/2019
	// 
	s32 buildingsRemaining = truncate32(visibleBuildings.count);
	Building *firstBuilding = *get(&visibleBuildings, 0);
	Asset *texture = getSprite(getBuildingDef(firstBuilding->typeID)->sprites, firstBuilding->spriteOffset)->texture;
	DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, texture, shaderID, buildingsRemaining);

	for (auto it = iterate(&visibleBuildings);
		!it.isDone;
		next(&it))
	{
		Building *building = getValue(it);

		if (typeID != building->typeID)
		{
			typeID = building->typeID;
			sprites = getBuildingDef(typeID)->sprites;
		}

		V4 drawColor = drawColorNormal;

		if (isDemolitionHappening && overlaps(building->footprint, demolitionRect))
		{
			// Draw building red to preview demolition
			drawColor = drawColorDemolish;
		}
		else if (!isEmpty(&building->problems))
		{
			if (building->problems & BuildingProblem_NoPower)
			{
				drawColor = drawColorNoPower;
			}
		}

		Sprite *sprite = getSprite(sprites, building->spriteOffset);

		if (texture == null)
		{
			texture = sprite->texture;
		}
		else if (texture != sprite->texture)
		{
			// Finish the current group and start a new one
			endRectsGroup(group);
			texture = sprite->texture;
			group = beginRectsGroupTextured(&renderer->worldBuffer, texture, shaderID, buildingsRemaining);
		}

		addSpriteRect(group, sprite, rect2(building->footprint), drawColor);
		buildingsRemaining--;
	}
	endRectsGroup(group);
}

// Runs an update on X sectors' buildings, gradually covering the whole city with subsequent calls.
void updateSomeBuildings(City *city)
{
	for (s32 i = 0; i < city->sectors.sectorsToUpdatePerTick; i++)
	{
		CitySector *sector = getNextSector(&city->sectors);

		for (auto it = iterate(&sector->ownedBuildings); !it.isDone; next(&it))
		{
			Building *building = getValue(it);
			BuildingDef *def = getBuildingDef(building->typeID);

			// Check the building's needs are met
			// ... except for the ones that are checked by layers.


			// Distance to road
			// TODO: Replace with access to any transport types, instead of just road? Not sure what we want with that.
			if ((def->flags & Building_RequiresTransportConnection) || (def->growsInZone))
			{
				s32 distanceToRoad = s32Max;
				// TODO: @Speed: We only actually need to check the boundary tiles, because they're guaranteed to be less than
				// the inner tiles... unless we allow multiple buildings per tile. Actually maybe we do? I'm not sure how that
				// would work really. Anyway, can think about that later.
				// - Sam, 30/08/2019
				for (s32 y = building->footprint.y; y < building->footprint.y + building->footprint.h; y++)
				{
					for (s32 x = building->footprint.x; x < building->footprint.x + building->footprint.w; x++)
					{
						distanceToRoad = min(distanceToRoad, getDistanceToTransport(city, x, y, Transport_Road));
					}
				}

				if (def->growsInZone)
				{
					// Zoned buildings inherit their zone's max distance to road.
					if (distanceToRoad > getZoneDef(def->growsInZone).maximumDistanceToRoad)
					{
						building->problems += BuildingProblem_NoTransportAccess;
					}
					else
					{
						building->problems -= BuildingProblem_NoTransportAccess;
					}
				}
				else if (def->flags & Building_RequiresTransportConnection)
				{
					// Other buildings require direct contact
					if (distanceToRoad > 1)
					{
						building->problems += BuildingProblem_NoTransportAccess;
					}
					else
					{
						building->problems -= BuildingProblem_NoTransportAccess;
					}
				}
			}
		}
	}
}
