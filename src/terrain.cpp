#pragma once

inline Terrain *getTerrainAt(City *city, s32 x, s32 y)
{
	Terrain *result = &invalidTerrain;
	if (tileExists(city, x, y))
	{
		result = getTile(city, city->tileTerrain, x, y);
	}

	return result;
}

inline u8 getTerrainHeightAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->tileTerrainHeight, x, y);
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

	s32 terrainType = -1;
	SpriteGroup *terrainSprites = null;

	Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
	V4 white = makeWhite();
	Array<V4> *palette = getPalette(makeString("terrain_height"));

	s32 tilesToDraw = areaOf(visibleArea);
	Asset *terrainTexture = getSprite(get(&terrainDefs, 1)->sprites, 0)->texture;
	DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, terrainTexture, shaderID, tilesToDraw);
	// DrawRectsGroup *group = beginRectsGroupUntextured(&renderer->worldBuffer, renderer->shaderIds.untextured, tilesToDraw);

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

			u8 terrainHeight = getTerrainHeightAt(city, x, y);

			addSpriteRect(group, sprite, spriteBounds, (*palette)[terrainHeight]);
			// addUntexturedRect(group, spriteBounds, (*palette)[terrainHeight]);
		}
	}
	endRectsGroup(group);
}

void generateTerrain(City *city)
{
	DEBUG_FUNCTION();
	
	u32 tGround = findTerrainTypeByName(makeString("Ground"));
	u32 tWater  = findTerrainTypeByName(makeString("Water"));
	BuildingDef *bTree = findBuildingDef(makeString("Tree"));

	fillMemory<u8>(city->tileDistanceToWater, 255, areaOf(city->bounds));

	Random terrainRandom;
	s32 seed = 0xdeadbeef;
	initRandom(&terrainRandom, Random_MT, seed);

	for (s32 y = 0; y < city->bounds.h; y++) {
		for (s32 x = 0; x < city->bounds.w; x++) {

			f32 px = (f32)x * 0.02f;
			f32 py = (f32)y * 0.02f;

			f32 perlinValue = 0.5f;
			// f32 perlinValue = stb_perlin_fbm_noise3(px, py, 0, 2.0f, 0.5f, 4);
			// f32 perlinValue = randomFloat01(&terrainRandom);

			setTile<u8>(city, city->tileTerrainHeight, x, y, clamp01AndMap_u8(perlinValue));

			Terrain *terrain = getTerrainAt(city, x, y);
			bool isWater = (perlinValue < 0.1f);
			if (isWater)
			{
				terrain->type = tWater;
				setTile<u8>(city, city->tileDistanceToWater, x, y, 0);
			}
			else
			{
				terrain->type = tGround;

				if (stb_perlin_noise3(px, py, 1, 0, 0, 0) > 0.2f)
				{
					Building *building = addBuilding(city, bTree, irectXYWH(x, y, bTree->width, bTree->height));
					building->spriteOffset = randomNext(&terrainRandom);
				}
			}

			terrain->spriteOffset = (s32) randomNext(&globalAppState.cosmeticRandom);
		}
	}

	// Generate a river
	initRandom(&terrainRandom, Random_MT, seed);
	Array<f32> riverOffset = allocateArray<f32>(tempArena, city->bounds.h);
	generate1DNoise(&terrainRandom, &riverOffset, 4);
	f32 riverMaxWidth = randomFloatBetween(&terrainRandom, 12, 16);
	f32 riverMinWidth = randomFloatBetween(&terrainRandom, 6, riverMaxWidth);
	for (s32 y=0; y < city->bounds.h; y++)
	{
		s32 riverWidth = ceil_s32(lerp(riverMinWidth, riverMaxWidth, ((f32)y / (f32)city->bounds.h)));
		s32 riverCentre = round_s32(city->bounds.w * (0.5f + (riverOffset[y] * 0.1f)));
		s32 riverLeft = riverCentre - (riverWidth / 2);

		for (s32 x = riverLeft; x < riverLeft + riverWidth; x++) 
		{
			Terrain *terrain = getTerrainAt(city, x, y);
			terrain->type = tWater;
			setTile<u8>(city, city->tileDistanceToWater, x, y, 0);
		}
	}

	updateDistances(city, city->tileDistanceToWater, city->bounds, maxDistanceToWater);
}
