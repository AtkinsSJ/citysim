#pragma once

void _assignBuildingCategories(BuildingCatalogue *catalogue, BuildingDef *def)
{
	if (def->buildMethod != BuildMethod_None)
	{
		append(&catalogue->constructibleBuildings, def);
	}

	switch(def->growsInZone)
	{
		case Zone_Residential: {
			append(&catalogue->rGrowableBuildings, def);
			catalogue->maxRBuildingDim = max(catalogue->maxRBuildingDim, max(def->width, def->height));
		} break;

		case Zone_Commercial: {
			append(&catalogue->cGrowableBuildings, def);
			catalogue->maxCBuildingDim = max(catalogue->maxCBuildingDim, max(def->width, def->height));
		} break;

		case Zone_Industrial: {
			append(&catalogue->iGrowableBuildings, def);
			catalogue->maxIBuildingDim = max(catalogue->maxIBuildingDim, max(def->width, def->height));
		} break;
	}
}

void loadBuildingDefs(AssetManager *assets, Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	BuildingCatalogue *catalogue = &buildingCatalogue;

	// @Cleanup DANGER! Currently, the asset system resets the assetarena, so we have to re-init these arrays every time.
	// That's gross, and if we ever DON'T reset that, this will be a leak! But oh well.
	ChunkedArray<BuildingDef> *buildings = &catalogue->buildingDefs;
	initChunkedArray(buildings, &assets->assetArena, 64);
	appendBlank(buildings);

	initChunkedArray(&catalogue->constructibleBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->rGrowableBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->cGrowableBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->iGrowableBuildings, &assets->assetArena, 64);

	catalogue->maxRBuildingDim = 0;
	catalogue->maxCBuildingDim = 0;
	catalogue->maxIBuildingDim = 0;


	BuildingDef *def = null;

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
			if (!equals(firstWord, "Building"))
			{
				warn(&reader, "Only Building definitions are supported right now.");
			}
			else
			{
				if (def != null)
				{
					// Now that the previous building is done, we can categorise it
					_assignBuildingCategories(catalogue, def);
				}

				def = appendBlank(buildings);
				def->name = pushString(&assets->assetArena, trimEnd(remainder));
				def->typeID = truncate32(buildings->count - 1);
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
					def->spriteName = readTextureDefinition(&reader, assets, remainder);
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
					def->demolishCost = (s32) readInt(&reader, firstWord, remainder);
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
					def->isPath = readBool(&reader, firstWord, remainder);
				}
				else if (equals(firstWord, "carries_power"))
				{
					def->carriesPower = readBool(&reader, firstWord, remainder);
				}
				else if (equals(firstWord, "power_gen"))
				{
					def->power = (s32) readInt(&reader, firstWord, remainder);
				}
				else if (equals(firstWord, "power_use"))
				{
					def->power = (s32) -readInt(&reader, firstWord, remainder);
				}
				else if (equals(firstWord, "residents"))
				{
					def->residents = (s32) readInt(&reader, firstWord, remainder);
				}
				else if (equals(firstWord, "jobs"))
				{
					def->jobs = (s32) readInt(&reader, firstWord, remainder);
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
	
	if (def != null)
	{
		// Categorise the last building
		_assignBuildingCategories(catalogue, def);
	}

	catalogue->isInitialised = true;

	logInfo("Loaded {0} buildings: {1} R, {2} C and {3} I growable, and {4} player-constructible", {
		formatInt(catalogue->buildingDefs.count),
		formatInt(catalogue->rGrowableBuildings.count),
		formatInt(catalogue->cGrowableBuildings.count),
		formatInt(catalogue->iGrowableBuildings.count),
		formatInt(catalogue->constructibleBuildings.count)
	});
}

inline BuildingDef *getBuildingDef(s32 buildingTypeID)
{
	return get(&buildingCatalogue.buildingDefs, buildingTypeID);
}

inline ChunkedArray<BuildingDef *> *getConstructibleBuildings()
{
	return &buildingCatalogue.constructibleBuildings;
}

inline ChunkedArray<BuildingDef *> *getRGrowableBuildings()
{
	return &buildingCatalogue.rGrowableBuildings;
}
inline ChunkedArray<BuildingDef *> *getCGrowableBuildings()
{
	return &buildingCatalogue.cGrowableBuildings;
}
inline ChunkedArray<BuildingDef *> *getIGrowableBuildings()
{
	return &buildingCatalogue.iGrowableBuildings;
}

void updateBuildingTexture(City *city, Building *building, BuildingDef *def)
{
	DEBUG_FUNCTION();

	if (building == null) return;

	if (def == null)
	{
		def = getBuildingDef(building->typeID);
	}

	if (def->linkTexturesLayer)
	{
		// Sprite id is 0 to 15, depending on connecting neighbours.
		// 1 = up, 2 = right, 4 = down, 8 = left
		// For now, we'll decide that off-the-map does NOT connect
		ASSERT(def->width == 1 && def->height == 1, "We only support texture-linking for 1x1 buildings!");
		
		s32 x = building->footprint.x;
		s32 y = building->footprint.y;

		switch (def->linkTexturesLayer)
		{
			case DataLayer_Paths:
			{
				Building *buildingU = getBuildingAtPosition(city, x,   y-1);
				Building *buildingD = getBuildingAtPosition(city, x,   y+1);
				Building *buildingL = getBuildingAtPosition(city, x-1, y  );
				Building *buildingR = getBuildingAtPosition(city, x+1, y  );

				bool linkU = buildingU && getBuildingDef(buildingU->typeID)->isPath;
				bool linkD = buildingD && getBuildingDef(buildingD->typeID)->isPath;
				bool linkL = buildingL && getBuildingDef(buildingL->typeID)->isPath;
				bool linkR = buildingR && getBuildingDef(buildingR->typeID)->isPath;

				building->spriteOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
			} break;

			case DataLayer_Power:
			{
				Building *buildingU = getBuildingAtPosition(city, x,   y-1);
				Building *buildingD = getBuildingAtPosition(city, x,   y+1);
				Building *buildingL = getBuildingAtPosition(city, x-1, y  );
				Building *buildingR = getBuildingAtPosition(city, x+1, y  );

				bool linkU = buildingU && getBuildingDef(buildingU->typeID)->carriesPower;
				bool linkD = buildingD && getBuildingDef(buildingD->typeID)->carriesPower;
				bool linkL = buildingL && getBuildingDef(buildingL->typeID)->carriesPower;
				bool linkR = buildingR && getBuildingDef(buildingR->typeID)->carriesPower;

				building->spriteOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
			} break;
		}
	}
	else
	{
		// Random sprite!
		building->spriteOffset = randomNext(&globalAppState.cosmeticRandom);
	}
}

void updateAdjacentBuildingTextures(City *city, Rect2I footprint)
{
	DEBUG_FUNCTION();

	for (s32 y = footprint.y;
		y < footprint.y + footprint.h;
		y++)
	{
		Building *buildingL = getBuildingAtPosition(city, footprint.x - 1, y);
		if (buildingL)
		{
			BuildingDef *defU = getBuildingDef(buildingL->typeID);
			if (defU->linkTexturesLayer) updateBuildingTexture(city, buildingL, defU);
		}

		Building *buildingR = getBuildingAtPosition(city, footprint.x + footprint.w, y);
		if (buildingR)
		{
			BuildingDef *defD = getBuildingDef(buildingR->typeID);
			if (defD->linkTexturesLayer) updateBuildingTexture(city, buildingR, defD);
		}
	}

	for (s32 x = footprint.x;
		x < footprint.x + footprint.w;
		x++)
	{
		Building *buildingU = getBuildingAtPosition(city, x, footprint.y - 1);
		Building *buildingD = getBuildingAtPosition(city, x, footprint.y + footprint.h);
		if (buildingU)
		{
			BuildingDef *defL = getBuildingDef(buildingU->typeID);
			if (defL->linkTexturesLayer) updateBuildingTexture(city, buildingU, defL);
		}
		if (buildingD)
		{
			BuildingDef *defR = getBuildingDef(buildingD->typeID);
			if (defR->linkTexturesLayer) updateBuildingTexture(city, buildingD, defR);
		}
	}
}

void refreshBuildingSpriteCache(BuildingCatalogue *catalogue, AssetManager *assets)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&catalogue->buildingDefs); !it.isDone; next(&it))
	{
		BuildingDef *def = get(it);

		// Account for the "null" building
		if (def->spriteName.length > 0)
		{
			def->sprites = getSpriteGroup(assets, def->spriteName);
		}
	}
}
