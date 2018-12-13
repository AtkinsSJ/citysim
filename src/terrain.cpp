#pragma once

void loadTerrainDefinitions(ChunkedArray<TerrainDef> *terrains, AssetManager *assets, File file)
{
	LineReader reader = startFile(file);

	initChunkedArray(terrains, &assets->assetArena, 16);
	appendBlank(terrains);

	TerrainDef *def = null;

	while (reader.pos < reader.file.length)
	{
		String line = nextLine(&reader);
		String firstWord;
		String remainder;

		firstWord = nextToken(line, &remainder);

		if (firstWord.chars[0] == ':') // Definitions
		{
			// Define something
			firstWord.chars++;
			firstWord.length--;

			String defType = firstWord;
			if (!equals(firstWord, "Terrain"))
			{
				warn(&reader, "Only Terrain definitions are supported right now.");
			}
			else
			{
				def = appendBlank(terrains);

				s64 terrainType;
				if (asInt(nextToken(remainder, &remainder), &terrainType))
				{
					def->type = (TerrainType) terrainType;
					def->name = pushString(&assets->assetArena, nextToken(remainder, &remainder));
				}
				else
				{
					error(&reader, "Couldn't parse TerrainType. Expected: ':Terrain type(int) name'");
					return;
				}
			}
		}
		else // Properties!
		{
			if (def == null)
			{
				error(&reader, "Found a property before starting a :Terrain!");
				return;
			}
			else
			{
				if (equals(firstWord, "texture"))
				{
					String textureName = nextToken(remainder, &remainder);
					s64 regionW;
					s64 regionH;
					s64 regionsAcross;
					s64 regionsDown;

					if (asInt(nextToken(remainder, &remainder), &regionW)
						&& asInt(nextToken(remainder, &remainder), &regionH)
						&& asInt(nextToken(remainder, &remainder), &regionsAcross)
						&& asInt(nextToken(remainder, &remainder), &regionsDown))
					{
						def->textureAssetType = addNewTextureAssetType(assets);
						addTiledTextureRegions(assets, def->textureAssetType, textureName, (u32)regionW, (u32)regionH, (u32)regionsAcross, (u32)regionsDown);
					}
					else
					{
						error(&reader, "Couldn't parse texture. Expected use: \"texture filename.png width height tilesAcross tilesDown\"");
						return;
					}
				}
				else if (equals(firstWord, "can_build_on"))
				{
					bool canBuildOn;
					if (asBool(nextToken(remainder, &remainder), &canBuildOn))
					{
						def->canBuildOn = canBuildOn;
					}
					else
					{
						error(&reader, "Couldn't parse can_build_on. Expected 1 boolean (true/false).");
						return;
					}
				}
				else if (equals(firstWord, "can_demolish"))
				{
					bool canDemolish;
					if (asBool(nextToken(remainder, &remainder), &canDemolish))
					{
						def->canDemolish = canDemolish;
						if (canDemolish)
						{
							s64 demolishCost;
							if (asInt(nextToken(remainder, &remainder), &demolishCost))
							{
								def->demolishCost = (s32) demolishCost;
							}
							else
							{
								error(&reader, "Couldn't parse can_demolish. Expected 'can_demolish true/false [cost]'.");
								return;
							}
						}
					}
					else
					{
						error(&reader, "Couldn't parse can_demolish. Expected 'can_demolish true/false [cost]'.");
						return;
					}
				}
			}
		}
	}

	// Make sure things match up, because they have to.
	for (s32 i = 0; i < Terrain_Size; i++)
	{
		ASSERT(i == get(terrains, i)->type, "Terrain data file must match the TerrainType enum for now!");
	}
}