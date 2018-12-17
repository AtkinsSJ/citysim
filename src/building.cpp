#pragma once

void loadBuildingDefs(ChunkedArray<BuildingDef> *buildings, AssetManager *assets, File file)
{
	LineReader reader = startFile(file);

	initChunkedArray(buildings, &assets->assetArena, 64);
	appendBlank(buildings);

	u32 defID = 0;
	BuildingDef *def = null;

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
				defID = buildings->itemCount;
				def = appendBlank(buildings);
				def->name = pushString(&assets->assetArena, trimEnd(remainder));
			}
		}
		else // Properties!
		{
			if (def == null)
			{
				error(&reader, "Found a property before starting a :Building!");
				return;
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
						return;
					}
				}
				else if (equals(firstWord, "texture"))
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
						error(&reader, "Couldn't parse texture. Expected use: \"texture filename.png width height (tilesAcross=1) (tilesDown=1)\"");
						return;
					}
				}
				else if (equals(firstWord, "build"))
				{
					String buildMethodString;
					s64 cost;

					buildMethodString = nextToken(remainder, &remainder);

					if (asInt(nextToken(remainder, &remainder), &cost))
					{
						if (equals(buildMethodString, "paint"))
						{
							def->buildMethod = BuildMethod_Paint;
						}
						else if (equals(buildMethodString, "plop"))
						{
							def->buildMethod = BuildMethod_Plop;
						}
						else if (equals(buildMethodString, "line"))
						{
							def->buildMethod = BuildMethod_DragLine;
						}
						else if (equals(buildMethodString, "rect"))
						{
							def->buildMethod = BuildMethod_DragRect;
						}
						else
						{
							warn(&reader, "Couldn't parse the build method, assuming NONE.");
							def->buildMethod = BuildMethod_None;
						}

						def->buildCost = (s32) cost;
					}
					else
					{
						error(&reader, "Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all.");
						return;
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
						return;
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
						return;
					}
				}
				else if (equals(firstWord, "carries_power"))
				{
					bool carriesPower;

					if (asBool(nextToken(remainder, &remainder), &carriesPower))
					{
						def->carriesPower = carriesPower;
					}
					else
					{
						error(&reader, "Couldn't parse carries_power. Expected 1 boolean (true/false).");
						return;
					}
				}
				else if (equals(firstWord, "power_gen"))
				{
					s64 power;

					if (asInt(nextToken(remainder, &remainder), &power))
					{
						def->power = (s32) power;
					}
					else
					{
						error(&reader, "Couldn't parse power_gen. Expected 1 int.");
						return;
					}
				}
				else if (equals(firstWord, "power_use"))
				{
					s64 power;

					if (asInt(nextToken(remainder, &remainder), &power))
					{
						def->power = (s32) -power;
					}
					else
					{
						error(&reader, "Couldn't parse power_use. Expected 1 int.");
						return;
					}
				}
				else if (equals(firstWord, "combination_of"))
				{
					String ingredient1;
					String ingredient2;

					if (splitInTwo(remainder, '|', &ingredient1, &ingredient2))
					{
						ingredient1 = trim(ingredient1);
						ingredient2 = trim(ingredient2);

						// Now, find the buildings so we can link it up!
						u32 ingredient1type = 0;
						u32 ingredient2type = 0;

						for (u32 typeID = 0; typeID < buildings->itemCount; typeID++)
						{
							BuildingDef *b = get(buildings, typeID);
							if (ingredient1type == 0 && equals(b->name, ingredient1))
							{
								ingredient1type = typeID;
							}
							else if (ingredient2type == 0 && equals(b->name, ingredient2))
							{
								ingredient2type = typeID;
							}

							if (ingredient1type && ingredient2type) break;
						}

						if (ingredient1type == 0)
						{
							error(&reader, "Couldn't locate building named \"{0}\", make sure it's defined in the file before this point!", {ingredient1});
							return;
						}
						else if (ingredient2type == 0)
						{
							error(&reader, "Couldn't locate building named \"{0}\", make sure it's defined in the file before this point!", {ingredient2});
							return;
						}
						else
						{
							// We found them, yay!
							BuildingDef *b1 = get(buildings, ingredient1type);
							BuildingDef *b2 = get(buildings, ingredient2type);
							if (b1->canBeBuiltOnID)
							{
								error(&reader, "Building named \"{0}\" is already involved in a combination. Right now, we only support one.", {ingredient1});
								return;
							}
							if (b2->canBeBuiltOnID)
							{
								error(&reader, "Building named \"{0}\" is already involved in a combination. Right now, we only support one.", {ingredient2});
								return;
							}
							b1->canBeBuiltOnID = ingredient2type;
							b2->canBeBuiltOnID = ingredient1type;
							b1->buildOverResult = b2->buildOverResult = defID;
						}
					}
					else
					{
						error(&reader, "Couldn't parse combination_of. Expected \"combination_of First Building Name | Second Building Name\".");
						return;
					}
				}
				else
				{
					error(&reader, "Unrecognized token: {0}", {firstWord});
				}
			}
		}
	}

	return;
}