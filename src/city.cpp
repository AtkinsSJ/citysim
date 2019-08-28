#pragma once

void initCity(MemoryArena *gameArena, Random *gameRandom, City *city, u32 width, u32 height, String name, s32 funds)
{
	*city = {};
	city->gameRandom = gameRandom;

	city->name = name;
	city->funds = funds;
	city->width = width;
	city->height = height;

	s32 cityArea = width * height;
	city->terrain           = allocateMultiple<Terrain>(gameArena, cityArea);
	city->tileBuildingIndex = allocateMultiple<s32>    (gameArena, cityArea);

	initChunkPool(&city->sectorBuildingsChunkPool,   gameArena, 128);
	initChunkPool(&city->sectorBoundariesChunkPool,  gameArena,   8);

	initSectorGrid(&city->sectors, gameArena, width, height, 16);
	for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&city->sectors); sectorIndex++)
	{
		CitySector *sector = &city->sectors.sectors[sectorIndex];
		initChunkedArray(&sector->buildingsOwned, &city->sectorBuildingsChunkPool);
	}

	initOccupancyArray(&city->buildings, gameArena, 1024);
	append(&city->buildings);

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

	CitySector *ownerSector = getSectorAtTilePos(&city->sectors, footprint.x, footprint.y);
	append(&ownerSector->buildingsOwned, building);

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
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

template<typename T>
inline T *getTile(City *city, T *tiles, s32 x, s32 y)
{
	return tiles + ((y * city->width) + x);
}

template<typename T>
inline T getTileValue(City *city, T *tiles, s32 x, s32 y)
{
	return tiles[((y * city->width) + x)];
}

template<typename T>
inline void setTile(City *city, T *tiles, s32 x, s32 y, T value)
{
	tiles[(y * city->width) + x] = value;
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
	if (!contains(irectXYWH(0,0, city->width, city->height), footprint))
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

void placeBuilding(City *city, BuildingDef *def, s32 left, s32 top)
{
	DEBUG_FUNCTION();

	Rect2I footprint = irectXYWH(left, top, def->width, def->height);

	bool needToRecalcTransport = (def->transportTypes != 0);
	bool needToRecalcPower = def->carriesPower;

	Building *building = getBuildingAt(city, left, top);
	if (building != null)
	{
		// Do a quick replace! We already established in canPlaceBuilding() that we match.
		// NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
		BuildingDef *oldDef = getBuildingDef(building->typeID);

		building->typeID = def->buildOverResult;
		def = getBuildingDef(def->buildOverResult);

		needToRecalcTransport = (oldDef->transportTypes != def->transportTypes);
		needToRecalcPower = (oldDef->carriesPower != def->carriesPower);

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

	if (needToRecalcTransport)
	{
		markTransportLayerDirty(&city->transportLayer, footprint);
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
					removeAllTransportFromTile(city, x, y); // We have to do this for building textures...
				}
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
				
				// Rebuild the buildingsOwned array
				clear(&sector->buildingsOwned);

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
								append(&sector->buildingsOwned, b);
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
			CitySector *sector = getSector(&city->sectors, sX, sY);

			for (auto it = iterate(&sector->buildingsOwned); !it.isDone; next(&it))
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
		result = getTile(city, city->terrain, x, y);
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
			Terrain *terrain = getTile(city, city->terrain, x, y);

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

	u32 buildingID = getTileValue(city, city->tileBuildingIndex, x, y);
	if (buildingID > 0)
	{
		result = get(&city->buildings, buildingID);
	}

	return result;
}

void drawBuildings(City *city, Rect2I visibleTileBounds, s8 shaderID, Rect2I demolitionRect)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	s32 typeID = -1;
	SpriteGroup *sprites = null;
	V4 drawColorNormal = makeWhite();
	V4 drawColorDemolish = color255(255,128,128,255);
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
