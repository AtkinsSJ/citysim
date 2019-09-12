#pragma once

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
