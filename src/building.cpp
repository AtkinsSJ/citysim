#pragma once

BuildingRef getReferenceTo(Building *building)
{
	BuildingRef result = {};
	result.buildingID = building->id;
	result.buildingPos = building->footprint.pos;

	return result;
}

Building *getBuilding(City *city, BuildingRef ref)
{
	Building *result = null;

	Building *buildingAtPosition = getBuildingAt(city, ref.buildingPos.x, ref.buildingPos.y);
	if ((buildingAtPosition != null) && (buildingAtPosition->id == ref.buildingID))
	{
		result = buildingAtPosition;
	}

	return result;
}

void _assignBuildingCategories(BuildingCatalogue *catalogue, BuildingDef *def)
{
	put(&catalogue->buildingsByName, def->name, def);

	catalogue->overallMaxBuildingDim = max(catalogue->overallMaxBuildingDim, max(def->width, def->height));

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

void loadBuildingDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	BuildingCatalogue *catalogue = &buildingCatalogue;
	ChunkedArray<BuildingDef> *buildings = &catalogue->allBuildings;
	HashTable<BuildingDef *> *buildingNames = &catalogue->buildingsByName;

	if (!isHashTableInitialised(buildingNames))
	{
		initHashTable(buildingNames, 0.75f, 128);
	}

	clear(buildingNames);

	// @Cleanup DANGER! Currently, the asset system resets the assetarena, so we have to re-init these arrays every time.
	// That's gross, and if we ever DON'T reset that, this will be a leak! But oh well.
	initChunkedArray(&catalogue->constructibleBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->rGrowableBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->cGrowableBuildings, &assets->assetArena, 64);
	initChunkedArray(&catalogue->iGrowableBuildings, &assets->assetArena, 64);

	initChunkedArray(buildings, &assets->assetArena, 64);
	// NB: BuildingDef ids are 1-indexed. At least one place (BuildingDef.canBeBuiltOnID) uses 0 as a "none" value.
	// So, we have to append a blank for a "null" def. Could probably get rid of it, but initialise-to-zero is convenient
	// and I'm likely to accidentally leave other things set to 0, so it's safer to just keep the null def.
	appendBlank(buildings);

	catalogue->maxRBuildingDim = 0;
	catalogue->maxCBuildingDim = 0;
	catalogue->maxIBuildingDim = 0;
	catalogue->overallMaxBuildingDim = 0;

	HashTable<BuildingDef> templates;
	initHashTable(&templates);
	defer { freeHashTable(&templates); };

	BuildingDef *def = null;

	while (loadNextLine(&reader))
	{
		String firstWord = readToken(&reader);

		if (firstWord[0] == ':') // Definitions
		{
			// Define something
			firstWord.chars++;
			firstWord.length--;

			if (equals(firstWord, "Building"))
			{
				if (def != null)
				{
					// Now that the previous building is done, we can categorise it
					_assignBuildingCategories(catalogue, def);
				}

				def = appendBlank(buildings);
				def->name = pushString(&assets->assetArena, getRemainderOfLine(&reader));
				def->typeID = truncate32(buildings->count - 1);
				initFlags(&def->flags, BuildingFlagCount);
				initFlags(&def->transportTypes, TransportTypeCount);

				def->fireRisk = 1.0f;
			}
			else if (equals(firstWord, "Template"))
			{
				String name = pushString(tempArena, getRemainderOfLine(&reader));
				def = put(&templates, name);
				initFlags(&def->flags, BuildingFlagCount);
				initFlags(&def->transportTypes, TransportTypeCount);
			}
			else
			{
				warn(&reader, "Only Building definitions are supported right now.");
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
				if (equals(firstWord, "build"))
				{
					String buildMethodString = readToken(&reader);
					Maybe<s64> cost = readInt(&reader);

					if (cost.isValid)
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

						def->buildCost = truncate32(cost.value);
					}
					else
					{
						error(&reader, "Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all.");
						return;
					}
				}
				else if (equals(firstWord, "carries_power"))
				{
					Maybe<bool> boolRead = readBool(&reader);
					if (boolRead.isValid)
					{
						if (boolRead.value)
						{
							def->flags += Building_CarriesPower;
						}
						else
						{
							def->flags -= Building_CarriesPower;
						}
					}
				}
				else if (equals(firstWord, "carries_transport"))
				{
					s32 tokenCount = countTokens(getRemainderOfLine(&reader));
					for (s32 tokenIndex = 0; tokenIndex < tokenCount; tokenIndex++)
					{
						String transportName = readToken(&reader);

						if (equals(transportName, "road"))
						{
							def->transportTypes += Transport_Road;
						}
						else if (equals(transportName, "rail"))
						{
							def->transportTypes += Transport_Rail;
						}
						else
						{
							warn(&reader, "Unrecognised transport type \"{0}\".", {transportName});
						}
					}
				}
				else if (equals(firstWord, "combination_of"))
				{
					// HACK: This is really janky, but this concept is going away soon anyway so who cares at this point!
					// - Sam, 12/09/2019
					String ingredientName1;
					String ingredientName2;

					if (splitInTwo(getRemainderOfLine(&reader), '|', &ingredientName1, &ingredientName2))
					{
						ingredientName1 = trim(ingredientName1);
						ingredientName2 = trim(ingredientName2);

						// Now, find the buildings so we can link it up!
						BuildingDef *ingredient1 = findBuildingDef(ingredientName1);
						BuildingDef *ingredient2 = findBuildingDef(ingredientName2);

						if (ingredient1 == null)
						{
							error(&reader, "Couldn't locate building named \"{0}\", make sure it's defined in the file before this point!", {ingredientName1});
							return;
						}
						else if (ingredient2 == null)
						{
							error(&reader, "Couldn't locate building named \"{0}\", make sure it's defined in the file before this point!", {ingredientName2});
							return;
						}
						else
						{
							// We found them, yay!
							if (ingredient1->canBeBuiltOnID)
							{
								error(&reader, "Building named \"{0}\" is already involved in a combination. Right now, we only support one.", {ingredientName1});
								return;
							}
							if (ingredient2->canBeBuiltOnID)
							{
								error(&reader, "Building named \"{0}\" is already involved in a combination. Right now, we only support one.", {ingredientName2});
								return;
							}
							ingredient1->canBeBuiltOnID = ingredient2->typeID;
							ingredient2->canBeBuiltOnID = ingredient1->typeID;
							ingredient1->buildOverResult = ingredient2->buildOverResult = def->typeID;
						}
					}
					else
					{
						error(&reader, "Couldn't parse combination_of. Expected \"combination_of First Building Name | Second Building Name\".");
						return;
					}
				}
				else if (equals(firstWord, "demolish_cost"))
				{
					Maybe<s64> demolish_cost = readInt(&reader);
					if (demolish_cost.isValid)
					{
						def->demolishCost = (s32) demolish_cost.value;
					}
				}
				else if (equals(firstWord, "extends"))
				{
					String templateName = readToken(&reader);
					
					BuildingDef *templateDef = find(&templates, templateName);
					if (templateDef == null)
					{
						error(&reader, "Could not find template named '{0}'. Make sure templates you use are defined before the buildings that use them!", {templateName});
					}
					else
					{
						// Copy the def... this could be messy
						// (We can't just do copyMemory() because we don't want to change the name or typeID.)
						def->flags = templateDef->flags;
						def->size = templateDef->size;
						def->spriteName = templateDef->spriteName;
						def->sprites = templateDef->sprites;
						def->linkTexturesLayer = templateDef->linkTexturesLayer;
						def->buildMethod = templateDef->buildMethod;
						def->buildCost = templateDef->buildCost;
						def->canBeBuiltOnID = templateDef->canBeBuiltOnID;
						def->buildOverResult = templateDef->buildOverResult;
						def->growsInZone = templateDef->growsInZone;
						def->demolishCost = templateDef->demolishCost;
						def->residents = templateDef->residents;
						def->jobs = templateDef->jobs;
						def->transportTypes = templateDef->transportTypes;
						def->power = templateDef->power;
						def->landValueEffect = templateDef->landValueEffect;
						def->pollutionEffect = templateDef->pollutionEffect;
						def->fireRisk = templateDef->fireRisk;
						def->fireProtection = templateDef->fireProtection;
					}
				}
				else if (equals(firstWord, "fire_protection"))
				{
					Maybe<EffectRadius> fire_protection = readEffectRadius(&reader);
					if (fire_protection.isValid)
					{
						def->fireProtection = fire_protection.value;
					}
				}
				else if (equals(firstWord, "fire_risk"))
				{
					Maybe<f64> fire_risk = readFloat(&reader);
					if (fire_risk.isValid)
					{
						def->fireRisk = (f32)fire_risk.value;
					}
				}
				else if (equals(firstWord, "grows_in"))
				{
					String zoneName = readToken(&reader);
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
				else if (equals(firstWord, "jobs"))
				{
					Maybe<s64> jobs = readInt(&reader);
					if (jobs.isValid)
					{
						def->jobs = (s32) jobs.value;
					}
				}
				else if (equals(firstWord, "land_value"))
				{
					Maybe<EffectRadius> land_value = readEffectRadius(&reader);
					if (land_value.isValid)
					{
						def->landValueEffect = land_value.value;
					}
				}
				else if (equals(firstWord, "link_textures"))
				{
					warn(&reader, "link_textures is disabled right now, because it's bad.");
					// String layer = readToken(&reader);
					// if (equals(layer, "path"))
					// {
					// 	def->linkTexturesLayer = DataLayer_Paths;
					// }
					// else if (equals(layer, "power"))
					// {
					// 	def->linkTexturesLayer = DataLayer_Power;
					// }
					// else
					// {
					// 	warn(&reader, "Couldn't parse link_textures, assuming NONE.");
					// }
				}
				else if (equals(firstWord, "pollution"))
				{
					Maybe<EffectRadius> pollution = readEffectRadius(&reader);
					if (pollution.isValid)
					{
						def->pollutionEffect = pollution.value;
					}
				}
				else if (equals(firstWord, "power_gen"))
				{
					Maybe<s64> power_gen = readInt(&reader);
					if (power_gen.isValid)
					{
						def->power = (s32) power_gen.value;
					}
				}
				else if (equals(firstWord, "power_use"))
				{
					Maybe<s64> power_use = readInt(&reader);
					if (power_use.isValid)
					{
						def->power = (s32) -power_use.value;
					}
				}
				else if (equals(firstWord, "requires_transport_connection"))
				{
					Maybe<bool> requires_transport_connection = readBool(&reader);
					if (requires_transport_connection.isValid)
					{
						if (requires_transport_connection.value)
						{
							def->flags += Building_RequiresTransportConnection;
						}
						else
						{
							def->flags -= Building_RequiresTransportConnection;
						}
					}
				}
				else if (equals(firstWord, "residents"))
				{
					Maybe<s64> residents = readInt(&reader);
					if (residents.isValid)
					{
						def->residents = (s32) residents.value;
					}
				}
				else if (equals(firstWord, "size"))
				{
					Maybe<s64> w = readInt(&reader);
					Maybe<s64> h = readInt(&reader);

					if (w.isValid && h.isValid)
					{
						def->width  = truncate32(w.value);
						def->height = truncate32(h.value);
					}
					else
					{
						error(&reader, "Couldn't parse size. Expected 2 ints (w,h).");
						return;
					}
				}
				else if (equals(firstWord, "texture"))
				{
					Maybe<String> spriteName = readTextureDefinition(&reader);
					if (spriteName.isValid)  def->spriteName = spriteName.value;
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

	logInfo("Loaded {0} buildings: {1} R, {2} C and {3} I growable, and {4} player-constructible", {
		formatInt(catalogue->allBuildings.count),
		formatInt(catalogue->rGrowableBuildings.count),
		formatInt(catalogue->cGrowableBuildings.count),
		formatInt(catalogue->iGrowableBuildings.count),
		formatInt(catalogue->constructibleBuildings.count)
	});
}

inline BuildingDef *getBuildingDef(s32 buildingTypeID)
{
	return get(&buildingCatalogue.allBuildings, buildingTypeID);
}

inline BuildingDef *getBuildingDef(Building *building)
{
	BuildingDef *result = null;
	
	if (building != null)
	{
		result = getBuildingDef(building->typeID);
	}

	return result;
}

inline BuildingDef *findBuildingDef(String name)
{
	BuildingDef *result = null;

	BuildingDef **findResult = find(&buildingCatalogue.buildingsByName, name);
	if (findResult != null)
	{
		result = *findResult;
	}

	return result;
}

inline ChunkedArray<BuildingDef *> *getConstructibleBuildings()
{
	return &buildingCatalogue.constructibleBuildings;
}

template<typename Filter>
BuildingDef *findRandomZoneBuilding(ZoneType zoneType, Random *random, Filter filter)
{
	DEBUG_FUNCTION();

	// Choose a random building, then carry on checking buildings until one is acceptable
	ChunkedArray<BuildingDef *> *buildings = null;
	switch (zoneType)
	{
		case Zone_Residential: buildings = &buildingCatalogue.rGrowableBuildings; break;
		case Zone_Commercial:  buildings = &buildingCatalogue.cGrowableBuildings; break;
		case Zone_Industrial:  buildings = &buildingCatalogue.iGrowableBuildings; break;

		INVALID_DEFAULT_CASE;
	}

	BuildingDef *result = null;

	// TODO: @RandomIterate - This random selection is biased, and wants replacing with an iteration only over valid options,
	// like in "growSomeZoneBuildings - find a valid zone".
	// Well, it does if growing buildings one at a time is how we want to do things. I'm not sure.
	// Growing a whole "block" of a building might make more sense for residential at least.
	// Something to decide on later.
	// - Sam, 18/08/2019
	for (auto it = iterate(buildings, randomBelow(random, truncate32(buildings->count)));
		!it.isDone;
		next(&it))
	{
		BuildingDef *def = getValue(it);

		if (filter(def))
		{
			result = def;
			break;
		}
	}

	return result;
}

s32 getMaxBuildingSize(ZoneType zoneType)
{
	s32 result = 0;

	switch (zoneType)
	{
		case Zone_Residential: result = buildingCatalogue.maxRBuildingDim; break;
		case Zone_Commercial:  result = buildingCatalogue.maxCBuildingDim; break;
		case Zone_Industrial:  result = buildingCatalogue.maxIBuildingDim; break;

		INVALID_DEFAULT_CASE;
	}

	return result;
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
		ASSERT(def->width == 1 && def->height == 1); //We only support texture-linking for 1x1 buildings!
		
		// s32 x = building->footprint.x;
		// s32 y = building->footprint.y;

		// switch (def->linkTexturesLayer)
		// {
		// 	case DataLayer_Paths:
		// 	{
		// 		bool linkU = doesTileHaveTransport(city, x,   y-1, def->transportTypes);
		// 		bool linkD = doesTileHaveTransport(city, x,   y+1, def->transportTypes);
		// 		bool linkL = doesTileHaveTransport(city, x-1, y,   def->transportTypes);
		// 		bool linkR = doesTileHaveTransport(city, x+1, y,   def->transportTypes);

		// 		building->spriteOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
		// 	} break;

		// 	case DataLayer_Power:
		// 	{
		// 		bool linkU = doesTileHavePowerNetwork(city, x,   y-1);
		// 		bool linkD = doesTileHavePowerNetwork(city, x,   y+1);
		// 		bool linkL = doesTileHavePowerNetwork(city, x-1, y  );
		// 		bool linkR = doesTileHavePowerNetwork(city, x+1, y  );

		// 		building->spriteOffset = (linkU ? 1 : 0) | (linkR ? 2 : 0) | (linkD ? 4 : 0) | (linkL ? 8 : 0);
		// 	} break;
		// }
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
		Building *buildingL = getBuildingAt(city, footprint.x - 1, y);
		if (buildingL)
		{
			BuildingDef *defU = getBuildingDef(buildingL->typeID);
			if (defU->linkTexturesLayer) updateBuildingTexture(city, buildingL, defU);
		}

		Building *buildingR = getBuildingAt(city, footprint.x + footprint.w, y);
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
		Building *buildingU = getBuildingAt(city, x, footprint.y - 1);
		Building *buildingD = getBuildingAt(city, x, footprint.y + footprint.h);
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

void refreshBuildingSpriteCache(BuildingCatalogue *catalogue)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&catalogue->allBuildings); !it.isDone; next(&it))
	{
		BuildingDef *def = get(it);

		// Account for the "null" building
		if (!isEmpty(def->spriteName))
		{
			def->sprites = getSpriteGroup(def->spriteName);
		}
	}
}

inline s32 getRequiredPower(Building *building)
{
	s32 result = 0;

	BuildingDef *def = getBuildingDef(building->typeID);
	if (def->power < 0)
	{
		result = -def->power;
	}

	return result;
}
