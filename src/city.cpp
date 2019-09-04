#pragma once

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->bounds = irectXYWH(0, 0, width, height);

	s32 cityArea = width * height;
	city->tileTerrain       = allocateMultiple<Terrain>(gameArena, cityArea);

	city->tileBuildingIndex = allocateMultiple<s32>    (gameArena, cityArea);
	initChunkPool(&city->sectorBuildingsChunkPool,   gameArena, 128);
	initChunkPool(&city->sectorBoundariesChunkPool,  gameArena,   8);

	initSectorGrid(&city->sectors, gameArena, width, height, 16);
	for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&city->sectors); sectorIndex++)
	{
		CitySector *sector = &city->sectors.sectors[sectorIndex];
		initChunkedArray(&sector->ownedBuildings, &city->sectorBuildingsChunkPool);
	}

	initOccupancyArray(&city->buildings, gameArena, 1024);
	append(&city->buildings);

	city->tileLandValue = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(city->tileLandValue, 128, cityArea);

	initPowerLayer    (&city->powerLayer,     city, gameArena);
	initTransportLayer(&city->transportLayer, city, gameArena);
	initZoneLayer     (&city->zoneLayer,      city, gameArena);

	city->highestBuildingID = 0;
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

	return building;
}

void generateTerrain(City *city)
{
	DEBUG_FUNCTION();
	
	u32 tGround = findTerrainTypeByName(makeString("Ground"));
	u32 tWater  = findTerrainTypeByName(makeString("Water"));

	// TODO: Replace this with a direct lookup!
	BuildingDef *bTree = findBuildingDef(makeString("Tree"));

	for (s32 y = 0; y < city->bounds.h; y++) {
		for (s32 x = 0; x < city->bounds.w; x++) {

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
					Building *building = addBuilding(city, bTree, irectXYWH(x, y, bTree->width, bTree->height));
					building->spriteOffset = randomNext(&globalAppState.cosmeticRandom);
				}
			}

			terrain->spriteOffset = (s32) randomNext(&globalAppState.cosmeticRandom);
		}
	}
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
			Terrain *terrain = getTerrainAt(city, x, y);
			TerrainDef *terrainDef = get(&terrainDefs, terrain->type);

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
		if (needToRecalcTransport)  markTransportLayerDirty(&city->transportLayer, footprint);
		if (needToRecalcPower)      markPowerLayerDirty(&city->powerLayer, footprint);
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

	bool needToRecalcTransport = !isEmpty(&def->transportTypes);
	bool needToRecalcPower = (def->flags & Building_CarriesPower);

	if (needToRecalcTransport)  markTransportLayerDirty(&city->transportLayer, area);
	if (needToRecalcPower)      markPowerLayerDirty(&city->powerLayer, area);
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
	{
		ChunkedArray<Building *> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
		for (auto it = iterate(&buildingsToDemolish, buildingsToDemolish.count-1, false, true);
			!it.isDone;
			next(&it))
		{
			Building *building = getValue(it);
			BuildingDef *def = getBuildingDef(building->typeID);

			city->zoneLayer.population[def->growsInZone] -= building->currentResidents + building->currentJobs;

			Rect2I buildingFootprint = building->footprint;

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
					// @Hack: We have to do this for updateAdjacentBuildingTextures()...
					// The whole buildingtextures system needs a rethink!
					removeAllTransportFromTile(city, x, y);
				}
			}

			markZonesAsEmpty(city, buildingFootprint);
			markPowerLayerDirty(&city->powerLayer, buildingFootprint);
			markTransportLayerDirty(&city->transportLayer, buildingFootprint);
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
		markPowerLayerDirty(&city->powerLayer, area);
		markTransportLayerDirty(&city->transportLayer, area);

		// Any buildings that would have connected with something that just got demolished need to refresh!
		updateAdjacentBuildingTextures(city, area);

	}
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

inline Terrain *getTerrainAt(City *city, s32 x, s32 y)
{
	Terrain *result = &invalidTerrain;
	if (tileExists(city, x, y))
	{
		result = getTile(city, city->tileTerrain, x, y);
	}

	return result;
}

void drawCity(City *city, Rect2I visibleTileBounds, Rect2I demolitionRect)
{
	drawTerrain(city, visibleTileBounds, renderer->shaderIds.pixelArt);

	drawZones(city, visibleTileBounds, renderer->shaderIds.untextured);

	drawBuildings(city, visibleTileBounds, renderer->shaderIds.pixelArt, demolitionRect);

	// Draw sectors
	// NB: this is really hacky debug code
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

void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	s32 terrainType = -1;
	SpriteGroup *terrainSprites = null;

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	V4 terrainColor = makeWhite();

	s32 tilesToDraw = areaOf(visibleArea);
	Asset *terrainTexture = getSprite(get(&terrainDefs, 1)->sprites, 0)->texture;
	DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, terrainTexture, shaderID, tilesToDraw);

	for (s32 y=visibleArea.y;
		y < visibleArea.y + visibleArea.h;
		y++)
	{
		spriteBounds.y = (f32) y;

		for (s32 x=visibleArea.x;
			x < visibleArea.x + visibleArea.w;
			x++)
		{
			Terrain *terrain = getTile(city, city->tileTerrain, x, y);

			if (terrain->type != terrainType)
			{
				terrainType = terrain->type;
				terrainSprites = get(&terrainDefs, terrainType)->sprites;
			}

			Sprite *sprite = getSprite(terrainSprites, terrain->spriteOffset);
			spriteBounds.x = (f32) x;

			addSpriteRect(group, sprite, spriteBounds, terrainColor);
		}
	}
	endRectsGroup(group);
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
	for (s32 i = 0; i < 8; i++)
	{
		CitySector *sector = &city->sectors[city->nextBuildingSectorUpdateIndex];

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

		city->nextBuildingSectorUpdateIndex = (city->nextBuildingSectorUpdateIndex + 1) % getSectorCount(&city->sectors);
	}
}

// The simplest possible algorithm is, just spread the 0s out that we marked above.
// (If a tile is not 0, set it to the min() of its 8 neighbours, plus 1.)
// We have to iterate through the area `maxDistance` times, but it should be fast enough probably.
void updateDistances(City *city, u8 *tileDistance, DirtyRects *dirtyRects, u8 maxDistance)
{
	for (s32 iteration = 0; iteration < maxDistance; iteration++)
	{
		for (auto it = iterate(&dirtyRects->rects);
			!it.isDone;
			next(&it))
		{
			Rect2I dirtyRect = getValue(it);

			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					if (getTileValue(city, tileDistance, x, y) != 0)
					{
						u8 minDistance = min({
							getTileValueIfExists<u8>(city, tileDistance, x-1, y-1, 255),
							getTileValueIfExists<u8>(city, tileDistance, x  , y-1, 255),
							getTileValueIfExists<u8>(city, tileDistance, x+1, y-1, 255),
							getTileValueIfExists<u8>(city, tileDistance, x-1, y  , 255),
						//	getTileValueIfExists<u8>(city, tileDistance, x  , y  , 255),
							getTileValueIfExists<u8>(city, tileDistance, x+1, y  , 255),
							getTileValueIfExists<u8>(city, tileDistance, x-1, y+1, 255),
							getTileValueIfExists<u8>(city, tileDistance, x  , y+1, 255),
							getTileValueIfExists<u8>(city, tileDistance, x+1, y+1, 255),
						});

						if (minDistance != 255)  minDistance++;
						if (minDistance > maxDistance)  minDistance = 255;

						setTile<u8>(city, tileDistance, x, y, minDistance);
					}
				}
			}
		}
	}
}

template<typename Filter>
s32 calculateDistanceTo(City *city, s32 x, s32 y, s32 maxDistanceToCheck, Filter filter)
{
	DEBUG_FUNCTION();
	
	s32 result = s32Max;

	if (filter(city, x, y))
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
				if (filter(city, px, bottomY))
				{
					result = distance;
					done = true;
					break;
				}

				if (filter(city, px, topY))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (filter(city, leftX, py))
				{
					result = distance;
					done = true;
					break;
				}

				if (filter(city, rightX, py))
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

void recalculateLandValue(City *city, Rect2I bounds)
{
	for (s32 y = bounds.y; y < bounds.y + bounds.h; y++)
	{
		for (s32 x = bounds.x; x < bounds.x + bounds.w; x++)
		{
			// Right now, we have very little to base this on!
			// This explains how SC3K does it: http://www.sc3000.com/knowledge/showarticle.cfm?id=1132
			// (However, apparently SC3K has an overflow bug with land value, so ehhhhhh...)
			// -> Maybe we should use a larger int or even a float for the land value. Memory is cheap!
			//
			// Anyway... being near water, and being near forest are positive.
			// Pollution, crime, good service coverage, and building effects all play their part.
			// So does terrain height if we ever implement non-flat terrain!
			//
			// Also, global effects can influence land value. AKA, is the city nice to live in?
			//
			// At some point it'd probably be sensible to cache some of the factors in intermediate arrays,
			// eg the effects of buildings, or distance to water, so we don't have to do a search for them for each tile.

			s32 waterTerrainType = findTerrainTypeByName(makeString("Water"));
			s32 distanceToWater = calculateDistanceTo(city, x, y, 10, [&](City *city, s32 x, s32 y) {
				return getTerrainAt(city, x, y)->type == waterTerrainType;
			});

			f32 landValue = 0.1f;

			if (distanceToWater < 10)
			{
				landValue += (10 - distanceToWater) * 0.1f * 0.25f;
			}

			setTile(city, city->tileLandValue, x, y, clamp01AndMap(landValue));
		}
	}
}

void drawLandValueDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	//
	// TODO: @Speed: Right now we're constructing an array that's just a copy of a section of the land value array,
	// and then passing that - when we could instead pass the whole array as a texture, and only draw the
	// visible part of it. (Or even draw the whole thing.)
	// We could pass the array as a texture, and the palette as a second texture, and then use a shader
	// similar to the one I developed for GLunatic.
	// But for right now, keep things simple.
	//
	// -Sam, 04/09/2019
	//

	u8 *data = copyRegion(city->tileLandValue, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);


	// TODO: Palette assets! Don't just recalculate this every time, that's ridiculous!

	V4 colorMinLandValue = color255(255, 255, 255, 128);
	V4 colorMaxLandValue = color255(  0,   0, 255, 128);

	V4 palette[256];
	f32 ratio = 1.0f / 255.0f;
	for (s32 i=0; i < 256; i++)
	{
		palette[i] = lerp(colorMinLandValue, colorMaxLandValue, i * ratio);
	}

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, 256, palette);
}

inline u8 getLandValueAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->tileLandValue, x, y);
}
