#pragma once

void loadBuildingDefinitions(Array<BuildingDefinition> *buildings, File file)
{
	LineReader reader = startFile(file);

	// Initialise the defs array if it hasn't been already
	if (buildings->maxCount == 0)
	{
		initialiseArray(buildings, 64);
	}

	clear(buildings);

	BuildingDefinition *def = null;

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
			if (!equals(firstWord, "Building"))
			{
				warn(&reader, "Only Building definitions are supported right now.");
			}
			else
			{
				def = appendBlank(buildings);
				*def = {};
				def->name = trimEnd(remainder);
			}
		}
		else // Properties!
		{
			if (def == null)
			{
				error(&reader, "Found a property before starting a :Building!");
			}
			else
			{
				if (equals(firstWord, "size"))
				{
					s64 w;
					s64 h;

					if (asInt(nextToken(remainder, &remainder), &w)
					 && asInt(nextToken(remainder, &remainder), &h))
					{
						def->width  = (s32) w;
						def->height = (s32) h;
					}
					else
					{
						error(&reader, "Couldn't parse size. Expected 2 ints (w,h).");
					}
				}
				else if (equals(firstWord, "texture"))
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
					}
				}
				else if (equals(firstWord, "build_cost"))
				{
					s64 cost;

					if (asInt(nextToken(remainder, &remainder), &cost))
					{
						def->buildCost = (s32) cost;
					}
					else
					{
						error(&reader, "Couldn't parse build_cost. Expected 1 int.");
					}
				}
				else if (equals(firstWord, "demolish_cost"))
				{
					s64 cost;

					if (asInt(nextToken(remainder, &remainder), &cost))
					{
						def->demolishCost = (s32) cost;
					}
					else
					{
						error(&reader, "Couldn't parse demolish_cost. Expected 1 int.");
					}
				}
				else if (equals(firstWord, "is_path"))
				{
					bool isPath;

					if (asBool(nextToken(remainder, &remainder), &isPath))
					{
						def->isPath = isPath;
					}
					else
					{
						error(&reader, "Couldn't parse is_path. Expected 1 boolean (true/false).");
					}
				}
			}
		}
	}
}