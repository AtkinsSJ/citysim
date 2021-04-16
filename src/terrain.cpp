#pragma once

void initTerrainLayer(TerrainLayer *layer, City *city, MemoryArena *gameArena)
{
	layer->tileTerrainType     = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	layer->tileHeight          = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	layer->tileDistanceToWater = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);

	layer->tileSpriteOffset    = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	layer->tileSprite          = allocateArray2<SpriteRef>(gameArena, city->bounds.w, city->bounds.h);
	layer->tileBorderSprite    = allocateArray2<SpriteRef>(gameArena, city->bounds.w, city->bounds.h);
}

inline TerrainDef *getTerrainAt(City *city, s32 x, s32 y)
{
	u8 terrainType = city->terrainLayer.tileTerrainType.getIfExists(x, y, 0);

	return getTerrainDef(terrainType);
}

inline u8 getTerrainHeightAt(City *city, s32 x, s32 y)
{
	return city->terrainLayer.tileHeight.get(x, y);
}

inline u8 getDistanceToWaterAt(City *city, s32 x, s32 y)
{
	return city->terrainLayer.tileDistanceToWater.get(x, y);
}

void initTerrainCatalogue()
{
	terrainCatalogue = {};

	initOccupancyArray(&terrainCatalogue.terrainDefs, &globalAppState.systemArena, 128);
	Indexed<TerrainDef*> nullTerrainDef = terrainCatalogue.terrainDefs.append();
	*nullTerrainDef.value = {};

	initHashTable(&terrainCatalogue.terrainDefsByName, 0.75f, 128);
	initStringTable(&terrainCatalogue.terrainNames);

	initHashTable(&terrainCatalogue.terrainNameToType, 0.75f, 128);
	terrainCatalogue.terrainNameToType.put(nullString, 0);

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

				Indexed<TerrainDef *> slot = terrainCatalogue.terrainDefs.append();
				def = slot.value;
				*def = {};

				if (slot.index > u8Max)
				{
					error(&reader, "Too many Terrain definitions! The most we support is {0}."_s, {formatInt(u8Max)});
					return;
				}
				def->typeID = (u8) slot.index;
				
				def->name = intern(&terrainCatalogue.terrainNames, name);
				asset->terrainDefs.terrainIDs.append(def->name);
				terrainCatalogue.terrainDefsByName.put(def->name, def);
				terrainCatalogue.terrainNameToType.put(def->name, def->typeID);
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
			else if (equals(firstWord, "borders"_s))
			{
				def->borderSpriteNames = allocateArray<String>(&assets->assetArena, 80);
			}
			else if (equals(firstWord, "border"_s))
			{
				def->borderSpriteNames.append(intern(&assets->assetStrings, readToken(&reader)));
			}
			else if (equals(firstWord, "can_build_on"_s))
			{
				Maybe<bool> boolRead = readBool(&reader);
				if (boolRead.isValid) def->canBuildOn = boolRead.value;
			}
			else if (equals(firstWord, "draw_borders_over"_s))
			{
				Maybe<bool> boolRead = readBool(&reader);
				if (boolRead.isValid) def->drawBordersOver = boolRead.value;
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
			terrainCatalogue.terrainDefs.removeIndex(terrainIndex);

			terrainCatalogue.terrainNameToType.removeKey(terrainName);
		}
	}
}

inline TerrainDef *getTerrainDef(u8 terrainType)
{
	TerrainDef *result = terrainCatalogue.terrainDefs.get(0);

	if (terrainType > 0 && terrainType < terrainCatalogue.terrainDefs.count)
	{
		TerrainDef *found = terrainCatalogue.terrainDefs.get(terrainType);
		if (found != null) result = found;
	}

	return result;
}

void refreshTerrainSpriteCache(TerrainCatalogue *catalogue)
{
	DEBUG_FUNCTION();

	for (auto it = catalogue->terrainDefs.iterate(); it.hasNext(); it.next())
	{
		TerrainDef *def = it.get();

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

	Maybe<TerrainDef *> def = terrainCatalogue.terrainDefsByName.findValue(name);
	if (def.isValid && def.value != null)
	{
		result = def.value->typeID;
	}

	return result;
}

void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	TerrainLayer *layer = &city->terrainLayer;

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	V4 white = makeWhite();

	// s32 tilesToDraw = areaOf(visibleArea);
	// Asset *terrainTexture = getSprite(getTerrainDef(1)->sprites, 0)->texture;
	// DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, terrainTexture, shaderID, tilesToDraw);

	for (s32 y=visibleArea.y;
		y < visibleArea.y + visibleArea.h;
		y++)
	{
		spriteBounds.y = (f32) y;

		for (s32 x=visibleArea.x;
			x < visibleArea.x + visibleArea.w;
			x++)
		{
			Sprite *sprite = getSprite(&layer->tileSprite.get(x,y));
			spriteBounds.x = (f32) x;
			drawSingleSprite(&renderer->worldBuffer, sprite, spriteBounds, shaderID, white);
			// addSpriteRect(group, sprite, spriteBounds, white);

			SpriteRef *borderSpriteRef = &layer->tileBorderSprite.get(x,y);
			if (!borderSpriteRef->spriteGroupName.isEmpty())
			{
				Sprite *borderSprite = getSprite(borderSpriteRef);
				drawSingleSprite(&renderer->worldBuffer, borderSprite, spriteBounds, shaderID, white);
				// addSpriteRect(group, borderSprite, spriteBounds, white);
			}
		}
	}
	// endRectsGroup(group);
}

void generateTerrain(City *city, Random *gameRandom)
{
	DEBUG_FUNCTION();

	TerrainLayer *layer = &city->terrainLayer;
	
	u8 tGround = truncate<u8>(findTerrainTypeByName("ground"_s));
	u8 tWater  = truncate<u8>(findTerrainTypeByName("water"_s));
	BuildingDef *treeDef = findBuildingDef("tree"_s);

	fill<u8>(&layer->tileDistanceToWater, 255);

	Random terrainRandom;
	s32 seed = randomNext(gameRandom);
	layer->terrainGenerationSeed = seed;
	initRandom(&terrainRandom, Random_MT, seed);
	fill<u8>(&layer->tileTerrainType, tGround);

	for (s32 y = 0; y < city->bounds.h; y++)
	{
		for (s32 x = 0; x < city->bounds.w; x++)
		{
			layer->tileSpriteOffset.set(x, y, randomInRange<u8>(&globalAppState.cosmeticRandom));
		}
	}

	// Generate a river
	Array<f32> riverOffset = allocateArray<f32>(tempArena, city->bounds.h, true);
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
			layer->tileTerrainType.set(x, y, tWater);
			layer->tileDistanceToWater.set(x, y, 0);
		}
	}

	// Coastline
	Array<f32> coastlineOffset = allocateArray<f32>(tempArena, city->bounds.w, true);
	generate1DNoise(&terrainRandom, &coastlineOffset, 10);
	for (s32 x=0; x < city->bounds.w; x++)
	{
		s32 coastDepth = 8 + round_s32(coastlineOffset[x] * 16.0f);

		for (s32 i=0; i < coastDepth; i++)
		{
			s32 y = city->bounds.h - 1 - i;

			layer->tileTerrainType.set(x, y, tWater);
			layer->tileDistanceToWater.set(x, y, 0);
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
					layer->tileTerrainType.set(x, y, tWater);
					layer->tileDistanceToWater.set(x, y, 0);
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

	assignTerrainTiles(city);

	updateDistances(&layer->tileDistanceToWater, city->bounds, maxDistanceToWater);
}

void assignTerrainTiles(City *city)
{
	TerrainLayer *layer = &city->terrainLayer;

	// TODO: assign a terrain tile variant for each tile, depending on its neighbours
	for (s32 y = 0; y < city->bounds.h; y++)
	{
		for (s32 x = 0; x < city->bounds.w; x++)
		{
			TerrainDef *def = getTerrainAt(city, x, y);

			layer->tileSprite.set(x, y, getSpriteRef(def->spriteName, layer->tileSpriteOffset.get(x, y)));
			
			if (def->drawBordersOver)
			{
				// First, determine if we have a bordering type
				TerrainDef *borders[8] = {}; // Starting NW, going clockwise
				borders[0] = getTerrainAt(city, x-1, y-1);
				borders[1] = getTerrainAt(city, x  , y-1);
				borders[2] = getTerrainAt(city, x+1, y-1);
				borders[3] = getTerrainAt(city, x+1, y  );
				borders[4] = getTerrainAt(city, x+1, y+1);
				borders[5] = getTerrainAt(city, x  , y+1);
				borders[6] = getTerrainAt(city, x-1, y+1);
				borders[7] = getTerrainAt(city, x-1, y  );

				// Find the first match, if any
				// Eventually we probably want to use whichever border terrain is most common instead
				TerrainDef *borderTerrain = null;
				for (s32 i=0; i < 8; i++)
				{
					if ((borders[i] != def) && (!borders[i]->borderSpriteNames.isEmpty()))
					{
						borderTerrain = borders[i];
						break;
					}
				}

				if (borderTerrain)
				{
					// Determine which border sprite to use based on our borders
					u8 n = (borders[1] == borderTerrain) ? 2 : (borders[0] == borderTerrain) ? 1 : 0;
					u8 e = (borders[3] == borderTerrain) ? 2 : (borders[2] == borderTerrain) ? 1 : 0;
					u8 s = (borders[5] == borderTerrain) ? 2 : (borders[4] == borderTerrain) ? 1 : 0;
					u8 w = (borders[7] == borderTerrain) ? 2 : (borders[6] == borderTerrain) ? 1 : 0;

					u8 borderSpriteIndex = w + (s * 3) + (e * 9) + (n * 27) - 1;
					ASSERT(borderSpriteIndex >= 0 && borderSpriteIndex <= 80);
					layer->tileBorderSprite.set(x, y, getSpriteRef(borderTerrain->borderSpriteNames[borderSpriteIndex], layer->tileSpriteOffset.get(x, y)));
				}
			}
		}
	}
}

void saveTerrainTypes()
{
	terrainCatalogue.terrainNameToOldType.putAll(&terrainCatalogue.terrainNameToType);
}

void remapTerrainTypes(City *city)
{
	// First, remap any Names that are not present in the current data, so they won't get
	// merged accidentally.
	for (auto it = terrainCatalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next())
	{
		auto entry = it.getEntry();
		if (!terrainCatalogue.terrainNameToType.contains(entry->key))
		{
			terrainCatalogue.terrainNameToType.put(entry->key, (u8)terrainCatalogue.terrainNameToType.count);
		}
	}

	if (terrainCatalogue.terrainNameToOldType.count > 0)
	{
		Array<u8> oldTypeToNewType = allocateArray<u8>(tempArena, terrainCatalogue.terrainNameToOldType.count, true);
		for (auto it = terrainCatalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next())
		{
			auto entry = it.getEntry();
			String terrainName = entry->key;
			u8 oldType       = entry->value;

			oldTypeToNewType[oldType] = terrainCatalogue.terrainNameToType.findValue(terrainName).orDefault(0);
		}

		TerrainLayer *layer = &city->terrainLayer;

		for (s32 y = 0; y < layer->tileTerrainType.h; y++)
		{
			for (s32 x = 0; x < layer->tileTerrainType.w; x++)
			{
				u8 oldType = layer->tileTerrainType.get(x, y);

				if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0))
				{
					layer->tileTerrainType.set(x, y, oldTypeToNewType[oldType]);
				}
			}
		}
	}

	saveTerrainTypes();
}
