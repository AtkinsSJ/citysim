#pragma once

void loadBuildingDefs(ChunkedArray<BuildingDef> *buildings, AssetManager *assets, File file)
{
	LineReader reader = startFile(file);

	initChunkedArray(buildings, &assets->assetArena, 64);
	appendBlank(buildings);

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
				def = appendBlank(buildings);
				def->name = pushString(&assets->assetArena, trimEnd(remainder));
				def->typeID = buildings->itemCount - 1;
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
				else if (equals(firstWord, "link_textures"))
				{
					String layer = nextToken(remainder, &remainder);
					if (equals(layer, "path"))
					{
						def->linkTexturesLayer = DataLayer_Paths;
					}
					else if (equals(layer, "power"))
					{
						def->linkTexturesLayer = DataLayer_Power;
					}
					else
					{
						warn(&reader, "Couldn't parse link_textures, assuming NONE.");
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
				else if (equals(firstWord, "grows_in"))
				{
					String zoneName = nextToken(remainder, &remainder);
					if (equals(zoneName, "r"))
					{
						def->growsInZone = Zone_Residential;
					}
					else if (equals(zoneName, "c"))
					{
						def->growsInZone = Zone_Commercial;
					}
					else if (equals(zoneName, "i"))
					{
						def->growsInZone = Zone_Industrial;
					}
					else
					{
						error(&reader, "Couldn't parse grows_in. Expected use:\"grows_in r/c/i\"");
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
				else if (equals(firstWord, "residents"))
				{
					s64 residents;

					if (asInt(nextToken(remainder, &remainder), &residents))
					{
						def->residents = (s32) residents;
					}
					else
					{
						error(&reader, "Couldn't parse residents. Expected 1 int.");
						return;
					}
				}
				else if (equals(firstWord, "jobs"))
				{
					s64 jobs;

					if (asInt(nextToken(remainder, &remainder), &jobs))
					{
						def->jobs = (s32) jobs;
					}
					else
					{
						error(&reader, "Couldn't parse jobs. Expected 1 int.");
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
						u32 ingredient1type = findBuildingTypeByName(ingredient1);
						u32 ingredient2type = findBuildingTypeByName(ingredient2);

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
							b1->buildOverResult = b2->buildOverResult = def->typeID;
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

void updatePathTexture(City *city, s32 x, s32 y)
{
	Building *building = getBuildingAtPosition(city, x, y);
	if (building)
	{
		bool linkU = isPathable(city, x,   y-1);
		bool linkD = isPathable(city, x,   y+1);
		bool linkL = isPathable(city, x-1, y  );
		bool linkR = isPathable(city, x+1, y  );

		building->textureRegionOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
	}
}

void updateBuildingTexture(City *city, Building *building, bool updateNeighbours, BuildingDef *def = null)
{
	if (building == null) return;

	if (def == null)
	{
		def = get(&buildingDefs, building->typeID);
	}

	s32 x = building->footprint.x;
	s32 y = building->footprint.y;
		
	if (def->linkTexturesLayer)
	{
		// Sprite id is 0 to 15, depending on connecting neighbours.
		// 1 = up, 2 = right, 4 = down, 8 = left
		// For now, we'll decide that off-the-map does NOT connect
		ASSERT(def->width == 1 && def->height == 1, "We only support 1x1 path buildings right now!");

		switch (def->linkTexturesLayer)
		{
			case DataLayer_Paths:
			{
				Building *buildingU = getBuildingAtPosition(city, x,   y-1);
				Building *buildingD = getBuildingAtPosition(city, x,   y+1);
				Building *buildingL = getBuildingAtPosition(city, x-1, y  );
				Building *buildingR = getBuildingAtPosition(city, x+1, y  );

				bool linkU = buildingU && get(&buildingDefs, buildingU->typeID)->isPath;
				bool linkD = buildingD && get(&buildingDefs, buildingD->typeID)->isPath;
				bool linkL = buildingL && get(&buildingDefs, buildingL->typeID)->isPath;
				bool linkR = buildingR && get(&buildingDefs, buildingR->typeID)->isPath;

				building->textureRegionOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
			} break;

			case DataLayer_Power:
			{
				Building *buildingU = getBuildingAtPosition(city, x,   y-1);
				Building *buildingD = getBuildingAtPosition(city, x,   y+1);
				Building *buildingL = getBuildingAtPosition(city, x-1, y  );
				Building *buildingR = getBuildingAtPosition(city, x+1, y  );

				bool linkU = buildingU && get(&buildingDefs, buildingU->typeID)->carriesPower;
				bool linkD = buildingD && get(&buildingDefs, buildingD->typeID)->carriesPower;
				bool linkL = buildingL && get(&buildingDefs, buildingL->typeID)->carriesPower;
				bool linkR = buildingR && get(&buildingDefs, buildingR->typeID)->carriesPower;

				building->textureRegionOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
			} break;
		}
	}
	else
	{
		// Random sprite!
		building->textureRegionOffset = randomNext(&globalAppState.cosmeticRandom);
	}

	if (updateNeighbours)
	{
		Building *buildingU = getBuildingAtPosition(city, x,   y-1);
		Building *buildingD = getBuildingAtPosition(city, x,   y+1);
		Building *buildingL = getBuildingAtPosition(city, x-1, y  );
		Building *buildingR = getBuildingAtPosition(city, x+1, y  );

		if (buildingU)
		{
			BuildingDef *defU = get(&buildingDefs, buildingU->typeID);
			if (defU->linkTexturesLayer) updateBuildingTexture(city, buildingU, false, defU);
		}
		if (buildingD)
		{
			BuildingDef *defD = get(&buildingDefs, buildingD->typeID);
			if (defD->linkTexturesLayer) updateBuildingTexture(city, buildingD, false, defD);
		}
		if (buildingL)
		{
			BuildingDef *defL = get(&buildingDefs, buildingL->typeID);
			if (defL->linkTexturesLayer) updateBuildingTexture(city, buildingL, false, defL);
		}
		if (buildingR)
		{
			BuildingDef *defR = get(&buildingDefs, buildingR->typeID);
			if (defR->linkTexturesLayer) updateBuildingTexture(city, buildingR, false, defR);
		}		
	}
}