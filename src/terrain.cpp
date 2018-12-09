#pragma once

void loadTerrainDefinitions(Array<TerrainDef> *terrains, File file)
{
	LineReader reader = startFile(file);

	// Initialise the defs array if it hasn't been already
	if (terrains->maxCount == 0)
	{
		initialiseArray(terrains, 16);
	}

	clear(terrains);

	TerrainDef nullTerrain = {Terrain_Invalid, TextureAssetType_None, false, false, 0};
	append(terrains, nullTerrain);

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
				}
				else
				{
					error(&reader, "Couldn't parse TerrainType. Expected: ':Terrain type(int)'");
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
					s64 textureAssetType;
					if (asInt(nextToken(remainder, &remainder), &textureAssetType))
					{
						/*
						 * TODO: Really need to be smarter with this, but right now we only have
						 * these defined as an enum in code, so we can't grab them by name.
						 */
						 def->textureAssetType = (TextureAssetType) textureAssetType;
					}
					else
					{
						error(&reader, "Couldn't parse texture. Expected 1 int.");
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
		ASSERT(i == (*terrains)[i].type, "Terrain data file must match the TerrainType enum for now!");
	}
}