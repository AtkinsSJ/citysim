#pragma once

void initCity(MemoryArena *gameArena, City *city, u32 width, u32 height, String name, String playerName, s32 funds)
{
	*city = {};

	// TODO: These want to be in some kind of buffer somewhere so they can be modified!
	city->name       = pushString(gameArena, name);
	city->playerName = pushString(gameArena, playerName);
	city->funds = funds;
	city->bounds = irectXYWH(0, 0, width, height);

	city->tileBuildingIndex = allocateArray2<s32>(gameArena, width, height);
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
	append(&city->buildings); // Null building

	initOccupancyArray(&city->entities, gameArena, 1024);

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

	// TODO: The rest of this code doesn't really belong here!
	// It belongs in a "we've just started/loaded a game, so initialise things" place.

	// TODO: Are we sure we want to do this?
	markAreaDirty(city, city->bounds);

	renderer->worldCamera.pos = v2(city->bounds.size) / 2;

	saveBuildingTypes();
	saveTerrainTypes();
}

template <typename T>
Entity *addEntity(City *city, EntityType type, T *entityData)
{
	auto entityRecord = append(&city->entities);
	// logInfo("Adding entity #{0}"_s, {formatInt(entityRecord.index)});
	entityRecord.value->index = entityRecord.index;

	Entity *entity = entityRecord.value;
	entity->type = type;
	// Make sure we're supplying entity data that matched the entity type!
	ASSERT(checkEntityMatchesType<T>(entity));
	entity->dataPointer = entityData;

	entity->color = makeWhite();
	entity->depth = 0;

	entity->canBeDemolished = false;

	return entity;
}

inline void removeEntity(City *city, Entity *entity)
{
	// logInfo("Removing entity #{0}"_s, {formatInt(entity->index)});
	removeIndex(&city->entities, entity->index);
}

void drawEntities(City *city, Rect2I visibleTileBounds)
{
	// TODO: Depth sorting
	// TODO: Sectors maybe? Though collecting all the visible entities into a data structure might be slower than just
	//  iterating the entities array.
	Rect2 cropArea = rect2(visibleTileBounds);
	auto shaderID = renderer->shaderIds.pixelArt;

	bool isDemolitionHappening = (areaOf(city->demolitionRect) > 0);
	V4 drawColorDemolish = color255(255,128,128,255);
	Rect2 demolitionRect = rect2(city->demolitionRect);

	for (auto it = iterate(&city->entities); hasNext(&it); next(&it))
	{
		auto entity = get(&it);
		if (overlaps(cropArea, entity->bounds))
		{
			// TODO: Batch these together somehow? Our batching is a bit complicated.
			// OK, turns out our renderer still batches same-texture-same-shader calls into a single draw call!
			// So, the difference is just sending N x RenderItem_DrawSingleRect instead of 1 x RenderItem_DrawRects
			// Thanks, past me!
			// - Sam, 26/09/2020

			V4 drawColor = entity->color;

			if (entity->canBeDemolished && isDemolitionHappening && overlaps(entity->bounds, demolitionRect))
			{
				drawColor *= drawColorDemolish;
			}

			drawSingleSprite(&renderer->worldBuffer, entity->sprite, entity->bounds, shaderID, drawColor);
		}
	}
}

Building *addBuildingDirect(City *city, s32 id, BuildingDef *def, Rect2I footprint, GameTimestamp creationDate)
{
	DEBUG_FUNCTION();

	auto buildingSlot = append(&city->buildings);
	s32 buildingIndex = buildingSlot.index;
	Building *building = buildingSlot.value;
	building->id = id;
	building->typeID = def->typeID;
	building->creationDate = creationDate;
	building->footprint = footprint;
	initFlags(&building->problems, BuildingProblemCount);
	building->variantIndex = NO_VARIANT;
	
	// Random sprite!
	building->spriteOffset = randomNext(&globalAppState.cosmeticRandom);

	building->entity = addEntity(city, EntityType_Building, building);
	building->entity->bounds = rect2(footprint);
	building->entity->sprite = getBuildingSprite(building);
	building->entity->canBeDemolished = true;

	CitySector *ownerSector = getSectorAtTilePos(&city->sectors, footprint.x, footprint.y);
	append(&ownerSector->ownedBuildings, building);

	for (s32 y = footprint.y;
		y < footprint.y + footprint.h;
		y++)
	{
		for (s32 x = footprint.x;
			x < footprint.x + footprint.w;
			x++)
		{
			city->tileBuildingIndex.set(x, y, buildingIndex);
		}
	}

	notifyNewBuilding(&city->crimeLayer, def, building);
	notifyNewBuilding(&city->fireLayer, def, building);
	notifyNewBuilding(&city->healthLayer, def, building);
	notifyNewBuilding(&city->powerLayer, def, building);

	return building;
}

Building *addBuilding(City *city, BuildingDef *def, Rect2I footprint, GameTimestamp creationDate)
{
	DEBUG_FUNCTION();

	Building *building = addBuildingDirect(city, ++city->highestBuildingID, def, footprint, creationDate);

	// TODO: Properly calculate occupancy!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

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
				Maybe<BuildingDef *> possibleIntersection = findBuildingIntersection(getBuildingDef(buildingAtPos->typeID), def);
				if (possibleIntersection.isValid)
				{
					// We can!
					// TODO: We want to check if there is a valid variant, before we build.
					// But that means matching against buildings that aren't constructed yet,
					// but just part of the drag-rect planned construction!
					// (eg, dragging a road across a rail, we want to check for a rail-crossing
					// variant that matches the planned road tiles.)
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

		Maybe<BuildingDef *> intersectionDef = findBuildingIntersection(oldDef, def);
		ASSERT(intersectionDef.isValid);

		building->typeID = intersectionDef.value->typeID;
		def = intersectionDef.value; // I really don't like this but I don't want to rewrite this entire function right now!

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

	// TODO: Calculate residents/jobs properly!
	building->currentResidents = def->residents;
	building->currentJobs = def->jobs;

	city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;

	updateBuildingVariant(city, building, def);
	updateAdjacentBuildingVariants(city, footprint);

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
	for (auto it = iterate(&buildingsToDemolish); hasNext(&it); next(&it))
	{
		Building *building = getValue(&it);
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
		hasNext(&it);
		next(&it))
	{
		Building *building = getValue(&it);
		BuildingDef *def = getBuildingDef(building->typeID);

		city->zoneLayer.population[def->growsInZone] -= building->currentResidents + building->currentJobs;

		Rect2I buildingFootprint = building->footprint;

		// Clean up other references
		notifyBuildingDemolished(&city->crimeLayer, def, building);
		notifyBuildingDemolished(&city->fireLayer, def, building);
		notifyBuildingDemolished(&city->healthLayer, def, building);
		notifyBuildingDemolished(&city->powerLayer, def, building);

		building->id = 0;
		building->typeID = -1;

		s32 buildingIndex = city->tileBuildingIndex.get(buildingFootprint.x, buildingFootprint.y);
		removeIndex(&city->buildings, buildingIndex);
		removeEntity(city, building->entity);

		building = null; // For safety, because we just deleted the Building!

		for (s32 y = buildingFootprint.y;
			y < buildingFootprint.y + buildingFootprint.h;
			y++)
		{
			for (s32 x = buildingFootprint.x;
				x < buildingFootprint.x + buildingFootprint.w;
				x++)
			{
				city->tileBuildingIndex.set(x, y, 0);
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
	updateAdjacentBuildingVariants(city, area);
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

			for (auto it = iterate(&sector->ownedBuildings); hasNext(&it); next(&it))
			{
				Building *building = getValue(&it);
				if (overlaps(building->footprint, area))
				{
					append(&result, building);
				}
			}
		}
	}

	return result;
}

void drawCity(City *city, Rect2I visibleTileBounds)
{
	drawTerrain(city, visibleTileBounds, renderer->shaderIds.pixelArt);

	drawZones(city, visibleTileBounds, renderer->shaderIds.untextured);

	// drawBuildings(city, visibleTileBounds, renderer->shaderIds.pixelArt, demolitionRect);

	drawEntities(city, visibleTileBounds);

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
	bool result = city->tileBuildingIndex.get(x, y) > 0;

	return result;
}

Building* getBuildingAt(City *city, s32 x, s32 y)
{
	Building *result = null;

	if (tileExists(city, x, y))
	{
		u32 buildingID = city->tileBuildingIndex.get(x, y);
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
	DrawRectsGroup *group = null;

	for (auto it = iterate(&visibleBuildings);
		hasNext(&it);
		next(&it))
	{
		Building *building = getValue(&it);

		Sprite *sprite = getBuildingSprite(building);

		// Skip buildings with no sprites (aka, the null building).
		// TODO: Give the null building a placeholder image to draw instead
		if (sprite != null)
		{
			if (group == null)
			{
				group = beginRectsGroupTextured(&renderer->worldBuffer, sprite->texture, shaderID, buildingsRemaining);
			}
			else if (group->texture != sprite->texture)
			{
				// Finish the current group and start a new one
				if (group != null)  endRectsGroup(group);
				group = beginRectsGroupTextured(&renderer->worldBuffer, sprite->texture, shaderID, buildingsRemaining);
			}

			V4 drawColor = drawColorNormal;

			if (isDemolitionHappening && overlaps(building->footprint, demolitionRect))
			{
				// Draw building red to preview demolition
				drawColor = drawColorDemolish;
			}
			else if (hasProblem(building, BuildingProblem_NoPower))
			{
				drawColor = drawColorNoPower;
			}

			addSpriteRect(group, sprite, rect2(building->footprint), drawColor);
		}

		buildingsRemaining--;
	}
	if (group != null)  endRectsGroup(group);
}

// Runs an update on X sectors' buildings, gradually covering the whole city with subsequent calls.
void updateSomeBuildings(City *city)
{
	for (s32 i = 0; i < city->sectors.sectorsToUpdatePerTick; i++)
	{
		CitySector *sector = getNextSector(&city->sectors);

		for (auto it = iterate(&sector->ownedBuildings); hasNext(&it); next(&it))
		{
			Building *building = getValue(&it);
			updateBuilding(city, building);
		}
	}
}
