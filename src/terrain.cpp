#pragma once

void loadTerrainDefinitions(ChunkedArray<TerrainDef> *terrains, AssetManager *assets, Blob data, Asset *asset)
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

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		if (line.length == 0) break;
		
		String firstWord;
		String remainder;

		firstWord = nextToken(line, &remainder);

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
				
				String name = trim(remainder);
				if (name.length == 0)
				{
					error(&reader, "Couldn't parse Terrain. Expected: ':Terrain name'");
					return;
				}
				def->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "Texture"))
			{
				mode = Mode_Texture;
				
				String filename = trim(remainder);
				if (filename.length == 0)
				{
					error(&reader, "Couldn't parse Texture. Expected: ':Terrain filename'");
					return;
				}
				textureAsset = addTexture(assets, filename, false);
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
						s64 spriteW;
						s64 spriteH;
						if (asInt(nextToken(remainder, &remainder), &spriteW)
							&& asInt(nextToken(remainder, &remainder), &spriteH))
						{
							spriteSize = v2i(truncate32(spriteW), truncate32(spriteH));
						}
						else
						{
							error(&reader, "Couldn't parse sprite_size. Expected 'sprite_size width height'.");
							return;
						}
					}
					else if (equals(firstWord, "sprite_border"))
					{
						s64 borderW;
						s64 borderH;
						if (asInt(nextToken(remainder, &remainder), &borderW)
							&& asInt(nextToken(remainder, &remainder), &borderH))
						{
							spriteBorder = v2i(truncate32(borderW), truncate32(borderH));
						}
						else
						{
							error(&reader, "Couldn't parse sprite_border. Expected 'sprite_border width height'.");
							return;
						}
					}
					else if (equals(firstWord, "sprite"))
					{
						String spriteName = pushString(&assets->assetArena, nextToken(remainder, &remainder));
						s64 x;
						s64 y;
						s64 w;
						s64 h;
						if (asInt(nextToken(remainder, &remainder), &x)
							&& asInt(nextToken(remainder, &remainder), &y)
							&& asInt(nextToken(remainder, &remainder), &w)
							&& asInt(nextToken(remainder, &remainder), &h))
						{
							Rect2I selectedSprites = irectXYWH(truncate32(x), truncate32(y), truncate32(w), truncate32(h));
							addTiledSprites(assets, spriteName, textureAsset, spriteSize, spriteBorder, selectedSprites);
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
						def->spriteName = pushString(&assets->assetArena, remainder);
					}
					else if (equals(firstWord, "can_build_on"))
					{
						def->canBuildOn = readBool(&reader, firstWord, remainder);
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

void refreshTerrainSpriteCache(ChunkedArray<TerrainDef> *terrains, AssetManager *assets)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(terrains); !it.isDone; next(&it))
	{
		TerrainDef *def = get(it);

		// Account for the "null" terrain
		if (def->spriteName.length > 0)
		{
			def->sprites = getSpriteGroup(assets, def->spriteName);
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
