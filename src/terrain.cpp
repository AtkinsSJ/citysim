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
				
				String name = trim(remainder);
				if (name.length == 0)
				{
					error(&reader, "Couldn't parse TerrainType. Expected: ':Terrain name'");
					return;
				}
				def->name = pushString(&assets->assetArena, name);
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
						&& asInt(nextToken(remainder, &remainder), &regionH))
					{
						if (!(asInt(nextToken(remainder, &remainder), &regionsAcross)
							&& asInt(nextToken(remainder, &remainder), &regionsDown)))
						{
							regionsAcross = 1;
							regionsDown = 1;
						}
						
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
}