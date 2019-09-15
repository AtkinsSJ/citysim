#pragma once

void initTerrainLayer(TerrainLayer *layer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = city->bounds.w * city->bounds.h;

	layer->tileTerrainType     = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileSpriteOffset    = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileTerrainHeight   = allocateMultiple<u8>(gameArena, cityArea);
	layer->tileDistanceToWater = allocateMultiple<u8>(gameArena, cityArea);
}

inline TerrainDef *getTerrainAt(City *city, s32 x, s32 y)
{
	s32 terrainType = 0;
	if (tileExists(city, x, y))
	{
		terrainType = getTileValue(city, city->terrainLayer.tileTerrainType, x, y);
	}

	TerrainDef *result = get(&terrainDefs, terrainType);
	return result;
}

inline u8 getTerrainHeightAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->terrainLayer.tileTerrainHeight, x, y);
}

inline u8 getDistanceToWaterAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->terrainLayer.tileDistanceToWater, x, y);
}

void loadTerrainDefs(ChunkedArray<TerrainDef> *terrains, Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	initChunkedArray(terrains, &assets->assetArena, 16);
	appendBlank(terrains);

	enum Mode
	{
		Mode_None = 0,
		Mode_Terrain,
		Mode_Texture,
	} mode = Mode_None;

	Asset *textureAsset = null;
	V2I spriteSize = v2i(0,0);
	V2I spriteBorder = v2i(0,0);

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
			if (equals(firstWord, "Terrain"))
			{
				mode = Mode_Terrain;
				def = appendBlank(terrains);
				
				String name = getRemainderOfLine(&reader);
				if (isEmpty(name))
				{
					error(&reader, "Couldn't parse Terrain. Expected: ':Terrain name'");
					return;
				}
				def->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "Texture"))
			{
				mode = Mode_Texture;
				
				String filename = getRemainderOfLine(&reader);
				if (isEmpty(filename))
				{
					error(&reader, "Couldn't parse Texture. Expected: ':Terrain filename'");
					return;
				}
				textureAsset = addTexture(filename, false);
			}
			else
			{
				error(&reader, "Unrecognised command: '{0}'", {firstWord});
			}
		}
		else // Properties!
		{
			switch (mode)
			{
				case Mode_Texture: {
					if (equals(firstWord, "sprite_size"))
					{
						Maybe<s64> spriteW = readInt(&reader);
						Maybe<s64> spriteH = readInt(&reader);
						if (spriteW.isValid && spriteH.isValid)
						{
							spriteSize = v2i(truncate32(spriteW.value), truncate32(spriteH.value));
						}
						else
						{
							error(&reader, "Couldn't parse sprite_size. Expected 'sprite_size width height'.");
							return;
						}
					}
					else if (equals(firstWord, "sprite_border"))
					{
						Maybe<s64> borderW = readInt(&reader);
						Maybe<s64> borderH = readInt(&reader);
						if (borderW.isValid && borderH.isValid)
						{
							spriteBorder = v2i(truncate32(borderW.value), truncate32(borderH.value));
						}
						else
						{
							error(&reader, "Couldn't parse sprite_border. Expected 'sprite_border width height'.");
							return;
						}
					}
					else if (equals(firstWord, "sprite"))
					{
						String spriteName = pushString(&assets->assetArena, readToken(&reader));
						Maybe<s64> x = readInt(&reader);
						Maybe<s64> y = readInt(&reader);
						Maybe<s64> w = readInt(&reader);
						Maybe<s64> h = readInt(&reader);

						if (x.isValid && y.isValid && w.isValid && h.isValid)
						{
							Rect2I selectedSprites = irectXYWH(truncate32(x.value), truncate32(y.value), truncate32(w.value), truncate32(h.value));
							addTiledSprites(spriteName, textureAsset, spriteSize, spriteBorder, selectedSprites);
						}
						else
						{
							error(&reader, "Couldn't parse sprite_border. Expected 'sprite_border width height'.");
							return;
						}
					}
					else
					{
						warn(&reader, "Unrecognised property '{0}' inside command ':Texture'", {firstWord});
					}
				} break;

				case Mode_Terrain: {
					if (equals(firstWord, "uses_sprite"))
					{
						def->spriteName = pushString(&assets->assetArena, getRemainderOfLine(&reader));
					}
					else if (equals(firstWord, "can_build_on"))
					{
						Maybe<bool> boolRead = readBool(&reader);
						if (boolRead.isValid) def->canBuildOn = boolRead.value;
					}
					else
					{
						warn(&reader, "Unrecognised property '{0}' inside command ':Terrain'", {firstWord});
					}
				} break;

				default: {
					error(&reader, "Found a property '{0}' before a :command!", {firstWord});
				} break;
			}
		}
	}
}

void refreshTerrainSpriteCache(ChunkedArray<TerrainDef> *terrains)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(terrains); !it.isDone; next(&it))
	{
		TerrainDef *def = get(it);

		// Account for the "null" terrain
		if (!isEmpty(def->spriteName))
		{
			def->sprites = getSpriteGroup(def->spriteName);
		}
	}
}

s32 findTerrainTypeByName(String name)
{
	DEBUG_FUNCTION();
	
	s32 result = 0;

	for (s32 terrainID = 1; terrainID < terrainDefs.count; terrainID++)
	{
		TerrainDef *def = get(&terrainDefs, terrainID);
		if (equals(def->name, name))
		{
			result = terrainID;
			break;
		}
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
			s32 type = getTileValue(city, layer->tileTerrainType, x, y);

			if (type != terrainType)
			{
				terrainType = type;
				terrainSprites = get(&terrainDefs, terrainType)->sprites;
			}

			Sprite *sprite = getSprite(terrainSprites, getTileValue(city, layer->tileSpriteOffset, x, y));
			spriteBounds.x = (f32) x;

			addSpriteRect(group, sprite, spriteBounds, white);
		}
	}
	endRectsGroup(group);
}

void generateTerrain(City *city)
{
	DEBUG_FUNCTION();

	TerrainLayer *layer = &city->terrainLayer;
	
	u8 tGround = truncate<u8>(findTerrainTypeByName(makeString("Ground")));
	u8 tWater  = truncate<u8>(findTerrainTypeByName(makeString("Water")));
	// BuildingDef *bTree = findBuildingDef(makeString("Tree"));

	fillMemory<u8>(layer->tileDistanceToWater, 255, areaOf(city->bounds));

	Random terrainRandom;
	s32 seed = randomNext(city->gameRandom);
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

	// TODO: Lakes/ponds
	s32 pondCount = randomBetween(&terrainRandom, 1, 4);
	for (s32 pondIndex = 0; pondIndex < pondCount; pondIndex++)
	{
		// Rough idea is: we pick a centre point, then produce a wrapped 1-d noise for the radius at each angle.
		// We then iterate through each tile in the bounding rect, and see if the distance is < the chosen radius
		// for that angle. If so, it's water!

		s32 pondCentreX = randomBetween(&terrainRandom, 0, city->bounds.w);
		s32 pondCentreY = randomBetween(&terrainRandom, 0, city->bounds.h);

		// For starters, we'll use a circle.
		Array<f32> pondRadiusPerAngle = allocateArray<f32>(tempArena, 36); // One for every 10 degrees, we'll interpolate anyway
		f32 angleToIndex = pondRadiusPerAngle.count / 360.0f;
		for (s32 i=0; i<pondRadiusPerAngle.count; i++) pondRadiusPerAngle[i] = 5.0f;

		s32 maxRadius = 5;
		Rect2I boundingBox = irectCentreSize(pondCentreX, pondCentreY, (maxRadius * 2) + 1, (maxRadius * 2) + 1);
		for (s32 y = boundingBox.y; y < boundingBox.y + boundingBox.h; y++)
		{
			for (s32 x = boundingBox.x; x < boundingBox.x + boundingBox.w; x++)
			{
				f32 angle = angleOf(x - pondCentreX, y - pondCentreY);
				f32 radiusAtAngle = pondRadiusPerAngle[floor_s32(angle * angleToIndex)];
				f32 distance = lengthOf(x - pondCentreX, y - pondCentreY);
				if (distance < radiusAtAngle)
				{
					setTile<u8>(city, layer->tileTerrainType, x, y, tWater);
					setTile<u8>(city, layer->tileDistanceToWater, x, y, 0);
				}
			}
		}
	}

	// TODO: Trees

	// TODO: assign a terrain tile variant for each tile, depending on its neighbours

	updateDistances(city, layer->tileDistanceToWater, city->bounds, maxDistanceToWater);
}
