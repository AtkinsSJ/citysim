#pragma once

void initTerrainLayer(TerrainLayer *layer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = city->bounds.w * city->bounds.h;

	layer->tileTerrainType     = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileSpriteOffset    = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileHeight          = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileDistanceToWater = allocateMultiple<u8>(gameArena, cityArea);
}

inline TerrainDef *getTerrainAt(City *city, s32 x, s32 y)
{
	u8 terrainType = 0;
	if (tileExists(city, x, y))
	{
		terrainType = getTileValue(city, city->terrainLayer.tileTerrainType, x, y);
	}

	TerrainDef *result = getTerrainDef(terrainType);
	return result;
}

inline u8 getTerrainHeightAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->terrainLayer.tileHeight, x, y);
}

inline u8 getDistanceToWaterAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->terrainLayer.tileDistanceToWater, x, y);
}

void initTerrainCatalogue()
{
	terrainCatalogue = {};

	initOccupancyArray(&terrainCatalogue.terrainDefs, &globalAppState.systemArena, 128);
	Indexed<TerrainDef*> nullTerrainDef = append(&terrainCatalogue.terrainDefs);
	*nullTerrainDef.value = {};

	initHashTable(&terrainCatalogue.terrainDefsByName, 0.75f, 128);
	initStringTable(&terrainCatalogue.terrainNames);

	initHashTable(&terrainCatalogue.terrainNameToType, 0.75f, 128);
	put<u8>(&terrainCatalogue.terrainNameToType, nullString, 0);

	initHashTable(&terrainCatalogue.terrainNameToOldType, 0.75f, 128);
}

void loadTerrainDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	// Pre scan for the number of Terrains, so we can allocate enough space in the asset.
	s32 terrainCount = 0;
	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);
		if (equals(command, ":Terrain"_s))
		{
			terrainCount++;
		}
	}

	asset->data = assetsAllocate(assets, sizeof(String) * terrainCount);
	asset->terrainDefs.terrainIDs = makeArray(terrainCount, (String *) asset->data.memory);
	s32 terrainIDsIndex = 0;

	restart(&reader);

	TerrainDef *def = null;

	while (loadNextLine(&reader))
	{
		String firstWord = readToken(&reader);

		if (firstWord.chars[0] == ':') // Definitions
		{
			// Define something
			firstWord.chars++;
			firstWord.length--;

			String defType = firstWord;
			if (equals(firstWord, "Terrain"_s))
			{
				String name = readToken(&reader);
				if (isEmpty(name))
				{
					error(&reader, "Couldn't parse Terrain. Expected: ':Terrain identifier'"_s);
					return;
				}

				Indexed<TerrainDef *> slot = append(&terrainCatalogue.terrainDefs);
				def = slot.value;

				if (slot.index > u8Max)
				{
					error(&reader, "Too many Terrain definitions! The most we support is {0}."_s, {formatInt(u8Max)});
					return;
				}
				def->typeID = (u8) slot.index;
				
				def->name = intern(&terrainCatalogue.terrainNames, name);
				asset->terrainDefs.terrainIDs[terrainIDsIndex++] = def->name;
				put(&terrainCatalogue.terrainDefsByName, def->name, def);
				put(&terrainCatalogue.terrainNameToType, def->name, def->typeID);
			}
			else
			{
				error(&reader, "Unrecognised command: '{0}'"_s, {firstWord});
			}
		}
		else // Properties!
		{
			if (def == null)
			{
				error(&reader, "Found a property before starting a :Terrain!"_s);
				return;
			}
			else if (equals(firstWord, "can_build_on"_s))
			{
				Maybe<bool> boolRead = readBool(&reader);
				if (boolRead.isValid) def->canBuildOn = boolRead.value;
			}
			else if (equals(firstWord, "name"_s))
			{
				def->textAssetName = intern(&assets->assetStrings, readToken(&reader));
			}
			else if (equals(firstWord, "sprite"_s))
			{
				def->spriteName = intern(&assets->assetStrings, readToken(&reader));
			}
			else
			{
				warn(&reader, "Unrecognised property '{0}' inside command ':Terrain'"_s, {firstWord});
			}
		}
	}
}

void removeTerrainDefs(Array<String> namesToRemove)
{
	for (s32 nameIndex = 0; nameIndex < namesToRemove.count; nameIndex++)
	{
		String terrainName = namesToRemove[nameIndex];
		s32 terrainIndex = findTerrainTypeByName(terrainName);
		if (terrainIndex > 0)
		{
			removeIndex(&terrainCatalogue.terrainDefs, terrainIndex);

			removeKey(&terrainCatalogue.terrainNameToType, terrainName);
		}
	}
}

inline TerrainDef *getTerrainDef(u8 terrainType)
{
	TerrainDef *result = get(&terrainCatalogue.terrainDefs, 0);

	if (terrainType > 0 && terrainType < terrainCatalogue.terrainDefs.count)
	{
		TerrainDef *found = get(&terrainCatalogue.terrainDefs, terrainType);
		if (found != null) result = found;
	}

	return result;
}

void refreshTerrainSpriteCache(TerrainCatalogue *catalogue)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&catalogue->terrainDefs); hasNext(&it); next(&it))
	{
		TerrainDef *def = get(&it);

		// Account for the "null" terrain
		if (!isEmpty(def->spriteName))
		{
			def->sprites = getSpriteGroup(def->spriteName);
		}
	}
}

u8 findTerrainTypeByName(String name)
{
	DEBUG_FUNCTION();
	
	u8 result = 0;

	TerrainDef **def = find(&terrainCatalogue.terrainDefsByName, name);
	if (def != null && *def != null)
	{
		result = (*def)->typeID;
	}

	return result;
}

void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	TerrainLayer *layer = &city->terrainLayer;

	s32 terrainType = -1;
	SpriteGroup *terrainSprites = null;

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	V4 white = makeWhite();

	s32 tilesToDraw = areaOf(visibleArea);
	Asset *terrainTexture = getSprite(getTerrainDef(1)->sprites, 0)->texture;
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
			s32 type = getTileValue(city, layer->tileTerrainType, x, y);

			if (type != terrainType)
			{
				terrainType = type;
				terrainSprites = getTerrainDef((u8)terrainType)->sprites;
			}

			// Null terrain has no sprites, so only draw if there's something to draw!
			if (terrainSprites != null)
			{
				Sprite *sprite = getSprite(terrainSprites, getTileValue(city, layer->tileSpriteOffset, x, y));
				spriteBounds.x = (f32) x;

				addSpriteRect(group, sprite, spriteBounds, white);
			}
		}
	}
	endRectsGroup(group);
}

void generateTerrain(City *city, Random *gameRandom)
{
	DEBUG_FUNCTION();

	TerrainLayer *layer = &city->terrainLayer;
	
	u8 tGround = truncate<u8>(findTerrainTypeByName("ground"_s));
	u8 tWater  = truncate<u8>(findTerrainTypeByName("water"_s));
	BuildingDef *treeDef = findBuildingDef("tree"_s);

	fillMemory<u8>(layer->tileDistanceToWater, 255, areaOf(city->bounds));

	Random terrainRandom;
	s32 seed = randomNext(gameRandom);
	layer->terrainGenerationSeed = seed;
	initRandom(&terrainRandom, Random_MT, seed);
	fillMemory<u8>(layer->tileTerrainType, tGround, areaOf(city->bounds));

	for (s32 y = 0; y < city->bounds.h; y++)
	{
		for (s32 x = 0; x < city->bounds.w; x++)
		{
			setTile<u8>(city, layer->tileSpriteOffset, x, y, randomInRange<u8>(&globalAppState.cosmeticRandom));
		}
	}

	// Generate a river
	Array<f32> riverOffset = allocateArray<f32>(tempArena, city->bounds.h);
	generate1DNoise(&terrainRandom, &riverOffset, 10);
	f32 riverMaxWidth = randomFloatBetween(&terrainRandom, 12, 16);
	f32 riverMinWidth = randomFloatBetween(&terrainRandom, 6, riverMaxWidth);
	f32 riverWaviness = 16.0f;
	s32 riverCentreBase = randomBetween(&terrainRandom, ceil_s32(city->bounds.w * 0.4f), floor_s32(city->bounds.w * 0.6f));
	for (s32 y=0; y < city->bounds.h; y++)
	{
		s32 riverWidth = ceil_s32(lerp(riverMinWidth, riverMaxWidth, ((f32)y / (f32)city->bounds.h)));
		s32 riverCentre = riverCentreBase - round_s32((riverWaviness * 0.5f) + (riverWaviness * riverOffset[y]));
		s32 riverLeft = riverCentre - (riverWidth / 2);

		for (s32 x = riverLeft; x < riverLeft + riverWidth; x++) 
		{
			setTile<u8>(city, layer->tileTerrainType, x, y, tWater);
			setTile<u8>(city, layer->tileDistanceToWater, x, y, 0);
		}
	}

	// Coastline
	Array<f32> coastlineOffset = allocateArray<f32>(tempArena, city->bounds.w);
	generate1DNoise(&terrainRandom, &coastlineOffset, 10);
	for (s32 x=0; x < city->bounds.w; x++)
	{
		s32 coastDepth = 8 + round_s32(coastlineOffset[x] * 16.0f);

		for (s32 i=0; i < coastDepth; i++)
		{
			s32 y = city->bounds.h - 1 - i;

			setTile<u8>(city, layer->tileTerrainType, x, y, tWater);
			setTile<u8>(city, layer->tileDistanceToWater, x, y, 0);
		}
	}

	// Lakes/ponds
	s32 pondCount = randomBetween(&terrainRandom, 1, 4);
	for (s32 pondIndex = 0; pondIndex < pondCount; pondIndex++)
	{
		s32 pondCentreX   = randomBetween(&terrainRandom, 0, city->bounds.w);
		s32 pondCentreY   = randomBetween(&terrainRandom, 0, city->bounds.h);

		f32 pondMinRadius = randomFloatBetween(&terrainRandom, 3.0f, 5.0f);
		f32 pondMaxRadius = randomFloatBetween(&terrainRandom, pondMinRadius + 3.0f, 20.0f);

		Splat pondSplat = createRandomSplat(pondCentreX, pondCentreY, pondMinRadius, pondMaxRadius, 36, &terrainRandom);

		Rect2I boundingBox = getBoundingBox(&pondSplat);
		for (s32 y = boundingBox.y; y < boundingBox.y + boundingBox.h; y++)
		{
			for (s32 x = boundingBox.x; x < boundingBox.x + boundingBox.w; x++)
			{
				if (contains(&pondSplat, x, y))
				{
					setTile<u8>(city, layer->tileTerrainType, x, y, tWater);
					setTile<u8>(city, layer->tileDistanceToWater, x, y, 0);
				}
			}
		}
	}

	// Forest splats
	if (treeDef == null)
	{
		logError("Map generator is unable to place any trees, because the 'tree' building was not found."_s);
	}
	else
	{
		s32 forestCount = randomBetween(&terrainRandom, 10, 20);
		for (s32 forestIndex = 0; forestIndex < forestCount; forestIndex++)
		{
			s32 centreX   = randomBetween(&terrainRandom, 0, city->bounds.w);
			s32 centreY   = randomBetween(&terrainRandom, 0, city->bounds.h);

			f32 minRadius = randomFloatBetween(&terrainRandom, 2.0f, 8.0f);
			f32 maxRadius = randomFloatBetween(&terrainRandom, minRadius + 1.0f, 30.0f);

			Splat forestSplat = createRandomSplat(centreX, centreY, minRadius, maxRadius, 36, &terrainRandom);

			Rect2I boundingBox = getBoundingBox(&forestSplat);
			for (s32 y = boundingBox.y; y < boundingBox.y + boundingBox.h; y++)
			{
				for (s32 x = boundingBox.x; x < boundingBox.x + boundingBox.w; x++)
				{
					if (getTerrainAt(city, x, y)->canBuildOn
						&& (getBuildingAt(city, x, y) == null)
						&& contains(&forestSplat, x, y))
					{
						addBuilding(city, treeDef, irectXYWH(x, y, treeDef->size.x, treeDef->size.y));
					}
				}
			}
		}
	}

	// TODO: assign a terrain tile variant for each tile, depending on its neighbours

	updateDistances(city, layer->tileDistanceToWater, city->bounds, maxDistanceToWater);
}

void saveTerrainTypes()
{
	putAll(&terrainCatalogue.terrainNameToOldType, &terrainCatalogue.terrainNameToType);
}

void remapTerrainTypes(City *city)
{
	// First, remap any Names that are not present in the current data, so they won't get
	// merged accidentally.
	for (auto it = iterate(&terrainCatalogue.terrainNameToOldType); hasNext(&it); next(&it))
	{
		auto entry = getEntry(&it);
		if (!contains(&terrainCatalogue.terrainNameToType, entry->key))
		{
			put(&terrainCatalogue.terrainNameToType, entry->key, (u8)terrainCatalogue.terrainNameToType.count);
		}
	}

	if (terrainCatalogue.terrainNameToOldType.count > 0)
	{
		Array<u8> oldTypeToNewType = allocateArray<u8>(tempArena, terrainCatalogue.terrainNameToOldType.count);
		for (auto it = iterate(&terrainCatalogue.terrainNameToOldType); hasNext(&it); next(&it))
		{
			auto entry = getEntry(&it);
			String terrainName = entry->key;
			u8 oldType       = entry->value;

			u8 *newType = find(&terrainCatalogue.terrainNameToType, terrainName);
			if (newType == null)
			{
				oldTypeToNewType[oldType] = 0;
			}
			else
			{
				oldTypeToNewType[oldType] = *newType;
			}
		}

		TerrainLayer *layer = &city->terrainLayer;

		s32 tileCount = areaOf(city->bounds);
		for (s32 tileIndex = 0; tileIndex < tileCount; tileIndex++)
		{
			u8 oldType = layer->tileTerrainType[tileIndex];

			if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0))
			{
				layer->tileTerrainType[tileIndex] = oldTypeToNewType[oldType];
			}
		}
	}

	saveTerrainTypes();
}
