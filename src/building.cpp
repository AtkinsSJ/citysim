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

void initBuildingCatalogue()
{
	BuildingCatalogue *catalogue = &buildingCatalogue;
	*catalogue = {};

	initChunkedArray(&catalogue->constructibleBuildings, &globalAppState.systemArena, 64);
	initChunkedArray(&catalogue->rGrowableBuildings,     &globalAppState.systemArena, 64);
	initChunkedArray(&catalogue->cGrowableBuildings,     &globalAppState.systemArena, 64);
	initChunkedArray(&catalogue->iGrowableBuildings,     &globalAppState.systemArena, 64);
	initChunkedArray(&catalogue->intersectionBuildings,  &globalAppState.systemArena, 64);

	initOccupancyArray(&catalogue->allBuildings, &globalAppState.systemArena, 64);
	// NB: BuildingDef ids are 1-indexed. At least one place (BuildingDef.canBeBuiltOnID) uses 0 as a "none" value.
	// So, we have to append a blank for a "null" def. Could probably get rid of it, but initialise-to-zero is convenient
	// and I'm likely to accidentally leave other things set to 0, so it's safer to just keep the null def.
	// Update 18/02/2020: We now use the null building def when failing to match an intersection part name.
	Indexed<BuildingDef*> nullBuildingDef = append(&catalogue->allBuildings);
	*nullBuildingDef.value = {};
	initFlags(&nullBuildingDef.value->flags, BuildingFlagCount);
	initFlags(&nullBuildingDef.value->transportTypes, TransportTypeCount);

	initHashTable(&catalogue->buildingsByName, 0.75f, 128);
	initStringTable(&catalogue->buildingNames);

	initHashTable(&catalogue->buildingNameToTypeID, 0.75f, 128);
	put<s32>(&catalogue->buildingNameToTypeID, nullString, 0);
	initHashTable(&catalogue->buildingNameToOldTypeID, 0.75f, 128);

	catalogue->maxRBuildingDim = 0;
	catalogue->maxCBuildingDim = 0;
	catalogue->maxIBuildingDim = 0;
	catalogue->overallMaxBuildingDim = 0;
}

void _assignBuildingCategories(BuildingCatalogue *catalogue, BuildingDef *def)
{
	if (def->typeID == 0) return; // Defs with typeID 0 are templates, which we don't want polluting the catalogue!

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

	if (def->isIntersection)
	{
		append(&catalogue->intersectionBuildings, def);
	}

	ASSERT(catalogue->allBuildings.count == (catalogue->buildingsByName.count + 1)); // NB: +1 for the null building.
}

BuildingDef *appendNewBuildingDef(String name)
{
	Indexed<BuildingDef *> newDef = append(&buildingCatalogue.allBuildings);
	BuildingDef *result = newDef.value;
	result->name = intern(&buildingCatalogue.buildingNames, name);
	result->typeID = newDef.index;
	initFlags(&result->flags, BuildingFlagCount);
	initFlags(&result->transportTypes, TransportTypeCount);

	result->fireRisk = 1.0f;
	put(&buildingCatalogue.buildingsByName, result->name, result);
	put(&buildingCatalogue.buildingNameToTypeID, result->name, result->typeID);

	return result;
}

void loadBuildingDefs(Blob data, Asset *asset)
{
	DEBUG_FUNCTION();

	LineReader reader = readLines(asset->shortName, data);

	BuildingCatalogue *catalogue = &buildingCatalogue;

	// Count the number of building defs in the file first, so we can allocate the buildingIDs array in the asset
	s32 buildingCount = 0;
	// Same for variants as they have their own structs
	s32 totalVariantCount = 0;
	while (loadNextLine(&reader))
	{
		String command = readToken(&reader);
		if (equals(command, ":Building"_s) || equals(command, ":Intersection"_s))
		{
			buildingCount++;
		}
		else if (equals(command, "variant"_s))
		{
			totalVariantCount++;
		}
	}

	smm buildingNamesSize = sizeof(String) * buildingCount;
	smm variantsSize = sizeof(BuildingVariant) * totalVariantCount;
	asset->data = assetsAllocate(assets, buildingNamesSize + variantsSize);
	asset->buildingDefs.buildingIDs = makeArray(buildingCount, (String *) asset->data.memory);
	u8 *variantsMemory = asset->data.memory + buildingNamesSize;
	s32 buildingIDsIndex = 0;
	s32 variantIndex = 0;

	restart(&reader);

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

			if (def != null)
			{
				// Now that the previous building is done, we can categorise it
				_assignBuildingCategories(catalogue, def);
			}

			if (equals(firstWord, "Building"_s))
			{
				String name = readToken(&reader);
				if (isEmpty(name))
				{
					error(&reader, "Couldn't parse Building. Expected: ':Building identifier'"_s);
					return;
				}

				def = appendNewBuildingDef(name);
				asset->buildingDefs.buildingIDs[buildingIDsIndex++] = def->name;
			}
			else if (equals(firstWord, "Intersection"_s))
			{
				String name = readToken(&reader);
				String part1Name = readToken(&reader);
				String part2Name = readToken(&reader);

				if (isEmpty(name) || isEmpty(part1Name) || isEmpty(part2Name))
				{
					error(&reader, "Couldn't parse Intersection. Expected: ':Intersection identifier part1 part2'"_s);
					return;
				}

				def = appendNewBuildingDef(name);
				asset->buildingDefs.buildingIDs[buildingIDsIndex++] = def->name;

				def->isIntersection = true;
				def->intersectionPart1Name = intern(&catalogue->buildingNames, part1Name);
				def->intersectionPart2Name = intern(&catalogue->buildingNames, part2Name);
			}
			else if (equals(firstWord, "Template"_s))
			{
				String name = readToken(&reader);
				if (isEmpty(name))
				{
					error(&reader, "Couldn't parse Template. Expected: ':Template identifier'"_s);
					return;
				}

				def = put(&templates, pushString(tempArena, name));
				initFlags(&def->flags, BuildingFlagCount);
				initFlags(&def->transportTypes, TransportTypeCount);
			}
			else
			{
				warn(&reader, "Only :Building, :Intersection or :Template definitions are supported right now."_s);
			}

			// Read ahead to count how many variants this building/intersection has.
			LineReaderPosition savedPosition = savePosition(&reader);
			s32 variantCount = 0;
			while (loadNextLine(&reader))
			{
				String _firstWord = readToken(&reader);
				if (_firstWord[0] == ':') break; // We've reached the next :Command

				if (equals(_firstWord, "variant"_s)) variantCount++;
			}
			restorePosition(&reader, savedPosition);

			if (variantCount > 0)
			{
				warn(&reader, "{0} variants"_s, {formatInt(variantCount)});
				def->variants = makeArray(variantCount, (BuildingVariant *)variantsMemory);
				variantsMemory += sizeof(BuildingVariant) * variantCount;
				variantIndex = 0;
			}

		}
		else // Properties!
		{
			if (def == null)
			{
				error(&reader, "Found a property before starting a :Building, :Intersection or :Template!"_s);
				return;
			}
			else
			{
				if (equals(firstWord, "build"_s))
				{
					String buildMethodString = readToken(&reader);
					Maybe<s32> cost = readInt<s32>(&reader);

					if (cost.isValid)
					{
						if (equals(buildMethodString, "paint"_s))
						{
							def->buildMethod = BuildMethod_Paint;
						}
						else if (equals(buildMethodString, "plop"_s))
						{
							def->buildMethod = BuildMethod_Plop;
						}
						else if (equals(buildMethodString, "line"_s))
						{
							def->buildMethod = BuildMethod_DragLine;
						}
						else if (equals(buildMethodString, "rect"_s))
						{
							def->buildMethod = BuildMethod_DragRect;
						}
						else
						{
							warn(&reader, "Couldn't parse the build method, assuming NONE."_s);
							def->buildMethod = BuildMethod_None;
						}

						def->buildCost = cost.value;
					}
					else
					{
						error(&reader, "Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all."_s);
						return;
					}
				}
				else if (equals(firstWord, "carries_power"_s))
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
				else if (equals(firstWord, "carries_transport"_s))
				{
					s32 tokenCount = countTokens(getRemainderOfLine(&reader));
					for (s32 tokenIndex = 0; tokenIndex < tokenCount; tokenIndex++)
					{
						String transportName = readToken(&reader);

						if (equals(transportName, "road"_s))
						{
							def->transportTypes += Transport_Road;
						}
						else if (equals(transportName, "rail"_s))
						{
							def->transportTypes += Transport_Rail;
						}
						else
						{
							warn(&reader, "Unrecognised transport type \"{0}\"."_s, {transportName});
						}
					}
				}
				else if (equals(firstWord, "crime_protection"_s))
				{
					Maybe<EffectRadius> crime_protection = readEffectRadius(&reader);
					if (crime_protection.isValid)
					{
						def->policeEffect = crime_protection.value;
					}
				}
				else if (equals(firstWord, "demolish_cost"_s))
				{
					Maybe<s32> demolish_cost = readInt<s32>(&reader);
					if (demolish_cost.isValid)
					{
						def->demolishCost = demolish_cost.value;
					}
				}
				else if (equals(firstWord, "extends"_s))
				{
					String templateName = readToken(&reader);
					
					BuildingDef *templateDef = find(&templates, templateName);
					if (templateDef == null)
					{
						error(&reader, "Could not find template named '{0}'. Templates must be defined before the buildings that use them, and in the same file."_s, {templateName});
					}
					else
					{
						// Copy the def... this could be messy
						// (We can't just do copyMemory() because we don't want to change the name or typeID.)
						def->flags = templateDef->flags;
						def->size = templateDef->size;
						def->spriteName = templateDef->spriteName;
						def->sprites = templateDef->sprites;
						def->buildMethod = templateDef->buildMethod;
						def->buildCost = templateDef->buildCost;
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
				else if (equals(firstWord, "fire_protection"_s))
				{
					Maybe<EffectRadius> fire_protection = readEffectRadius(&reader);
					if (fire_protection.isValid)
					{
						def->fireProtection = fire_protection.value;
					}
				}
				else if (equals(firstWord, "fire_risk"_s))
				{
					Maybe<f64> fire_risk = readFloat(&reader);
					if (fire_risk.isValid)
					{
						def->fireRisk = (f32)fire_risk.value;
					}
				}
				else if (equals(firstWord, "grows_in"_s))
				{
					String zoneName = readToken(&reader);
					if (equals(zoneName, "r"_s))
					{
						def->growsInZone = Zone_Residential;
					}
					else if (equals(zoneName, "c"_s))
					{
						def->growsInZone = Zone_Commercial;
					}
					else if (equals(zoneName, "i"_s))
					{
						def->growsInZone = Zone_Industrial;
					}
					else
					{
						error(&reader, "Couldn't parse grows_in. Expected use:\"grows_in r/c/i\""_s);
						return;
					}
				}
				else if (equals(firstWord, "health_effect"_s))
				{
					Maybe<EffectRadius> health_effect = readEffectRadius(&reader);
					if (health_effect.isValid)
					{
						def->healthEffect = health_effect.value;
					}
				}
				else if (equals(firstWord, "jail_size"_s))
				{
					Maybe<s32> jail_size = readInt<s32>(&reader);
					if (jail_size.isValid)
					{
						def->jailCapacity = jail_size.value;
					}
				}
				else if (equals(firstWord, "jobs"_s))
				{
					Maybe<s32> jobs = readInt<s32>(&reader);
					if (jobs.isValid)
					{
						def->jobs = jobs.value;
					}
				}
				else if (equals(firstWord, "land_value"_s))
				{
					Maybe<EffectRadius> land_value = readEffectRadius(&reader);
					if (land_value.isValid)
					{
						def->landValueEffect = land_value.value;
					}
				}
				else if (equals(firstWord, "name"_s))
				{
					def->textAssetName = intern(&assets->assetStrings, readToken(&reader));
				}
				else if (equals(firstWord, "pollution"_s))
				{
					Maybe<EffectRadius> pollution = readEffectRadius(&reader);
					if (pollution.isValid)
					{
						def->pollutionEffect = pollution.value;
					}
				}
				else if (equals(firstWord, "power_gen"_s))
				{
					Maybe<s32> power_gen = readInt<s32>(&reader);
					if (power_gen.isValid)
					{
						def->power = power_gen.value;
					}
				}
				else if (equals(firstWord, "power_use"_s))
				{
					Maybe<s32> power_use = readInt<s32>(&reader);
					if (power_use.isValid)
					{
						def->power = -power_use.value;
					}
				}
				else if (equals(firstWord, "requires_transport_connection"_s))
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
				else if (equals(firstWord, "residents"_s))
				{
					Maybe<s32> residents = readInt<s32>(&reader);
					if (residents.isValid)
					{
						def->residents = residents.value;
					}
				}
				else if (equals(firstWord, "size"_s))
				{
					Maybe<s32> w = readInt<s32>(&reader);
					Maybe<s32> h = readInt<s32>(&reader);

					if (w.isValid && h.isValid)
					{
						def->width  = w.value;
						def->height = h.value;

						if ((def->variants.count > 0) && (def->width != 1 || def->height != 1))
						{
							error(&reader, "This building is {0}x{1} and has variants. Variants are only allowed for 1x1 tile buildings!"_s, {formatInt(def->width), formatInt(def->height)});
							return;
						}
					}
					else
					{
						error(&reader, "Couldn't parse size. Expected 2 ints (w,h)."_s);
						return;
					}
				}
				else if (equals(firstWord, "sprite"_s))
				{
					String spriteName = intern(&assets->assetStrings, readToken(&reader));
					def->spriteName = spriteName;
				}
				else if (equals(firstWord, "variant"_s))
				{
					//
					// NB: Not really related to this code but I needed somewhere to put this:
					// Right now, we linerarly search through variants and pick the first one
					// that matches. (And though I originally expected some kind of map, that was
					// before we had 4 different states per connection, which prevents that.)
					// So, this makes the order important! If multiple variants can match a
					// situation, then the more specific one needs to come first.
					//
					// eg, if you have an "anything in all directions" variant, and it's first,
					// then nothing else will ever get chosen!
					//
					// I'm not sure if this is actually a problem, but it's something to keep in
					// mind. Maybe we could check all the variants when matching, and choose the
					// most specific one, which is calculated somehow. IDK. That would mean having
					// to check every variant, instead of stopping once we find one.
					//
					// - Sam, 19/02/2020
					//

					if (variantIndex < def->variants.count)
					{
						BuildingVariant *variant = def->variants.items + variantIndex;
						variantIndex++;

						*variant = {};

						String directionFlags = readToken(&reader);
						String spriteName = readToken(&reader);

						if (directionFlags.length == 8)
						{
							variant->connections[Connect_N ] = connectionTypeOf(directionFlags[0]);
							variant->connections[Connect_NE] = connectionTypeOf(directionFlags[1]); 
							variant->connections[Connect_E ] = connectionTypeOf(directionFlags[2]); 
							variant->connections[Connect_SE] = connectionTypeOf(directionFlags[3]);
							variant->connections[Connect_S ] = connectionTypeOf(directionFlags[4]);
							variant->connections[Connect_SW] = connectionTypeOf(directionFlags[5]); 
							variant->connections[Connect_W ] = connectionTypeOf(directionFlags[6]); 
							variant->connections[Connect_NW] = connectionTypeOf(directionFlags[7]);
						}
						else if (directionFlags.length == 4)
						{
							// The 4 other directions don't matter
							variant->connections[Connect_NE] = ConnectionType_Anything;
							variant->connections[Connect_SE] = ConnectionType_Anything; 
							variant->connections[Connect_SW] = ConnectionType_Anything; 
							variant->connections[Connect_NW] = ConnectionType_Anything;

							variant->connections[Connect_N] = connectionTypeOf(directionFlags[0]);
							variant->connections[Connect_E] = connectionTypeOf(directionFlags[1]); 
							variant->connections[Connect_S] = connectionTypeOf(directionFlags[2]); 
							variant->connections[Connect_W] = connectionTypeOf(directionFlags[3]);
						}
						else
						{
							error(&reader, "First argument for a building 'variant' should be a 4 or 8 character string consisting of 0/1/2/* flags (meaning nothing/part1/part2/anything) for N/E/S/W or N/NE/E/SE/S/SW/W/NW connectivity. eg, 101012**"_s);
							return;
						}

						String connectionsString = repeatChar(' ', ConnectionDirectionCount);
						for (s32 connectionIndex = 0; connectionIndex < ConnectionDirectionCount; connectionIndex++)
						{
							if (variant->connections[connectionIndex] == ConnectionType_Invalid)
							{
								error(&reader, "Unrecognized connection type character, valid values: '012*'"_s);
							}

							connectionsString[connectionIndex] = asChar(variant->connections[connectionIndex]);
						}

						logInfo("{0} variant {1} has connections {2} (from '{3}')"_s, {def->name, formatInt(variantIndex), connectionsString, directionFlags});

						variant->spriteName = spriteName;
					}
					else
					{
						error(&reader, "Too many variants for building '{0}'!"_s, {def->name});
					}
				}
				else
				{
					error(&reader, "Unrecognized token: {0}"_s, {firstWord});
				}
			}
		}
	}
	
	if (def != null)
	{
		// Categorise the last building
		_assignBuildingCategories(catalogue, def);
	}

	logInfo("Loaded {0} buildings: R:{1} C:{2} I:{3} growable, player-constructible:{4}, intersections:{5}"_s, {
		formatInt(catalogue->allBuildings.count),
		formatInt(catalogue->rGrowableBuildings.count),
		formatInt(catalogue->cGrowableBuildings.count),
		formatInt(catalogue->iGrowableBuildings.count),
		formatInt(catalogue->constructibleBuildings.count),
		formatInt(catalogue->intersectionBuildings.count)
	});
}

void removeBuildingDefs(Array<String> idsToRemove)
{
	BuildingCatalogue *catalogue = &buildingCatalogue;

	for (s32 idIndex = 0; idIndex < idsToRemove.count; idIndex++)
	{
		String buildingID = idsToRemove[idIndex];
		BuildingDef *def = findBuildingDef(buildingID);
		if (def != null)
		{
			findAndRemove(&catalogue->constructibleBuildings, def);
			findAndRemove(&catalogue->rGrowableBuildings,     def);
			findAndRemove(&catalogue->cGrowableBuildings,     def);
			findAndRemove(&catalogue->iGrowableBuildings,     def);
			findAndRemove(&catalogue->intersectionBuildings,  def);

			removeKey(&catalogue->buildingsByName, buildingID);

			removeIndex(&catalogue->allBuildings, def->typeID);

			removeKey(&catalogue->buildingNameToTypeID, buildingID);
		}
	}

	// TODO: How/when do we recalculate these?
	// I guess as the max building sizes are an optimisation, and this code is only
	// run either during development or when users are developing mods, it's not a big
	// deal if we keep the old values. They're guaranteed to be >= the real value,
	// which is all that matters for correctness.
	// - Sam, 10/11/2019
	//
	// catalogue->maxRBuildingDim = 0;
	// catalogue->maxCBuildingDim = 0;
	// catalogue->maxIBuildingDim = 0;
	// catalogue->overallMaxBuildingDim = 0;
}

inline BuildingDef *getBuildingDef(s32 buildingTypeID)
{
	BuildingDef *result = get(&buildingCatalogue.allBuildings, 0);

	if (buildingTypeID > 0 && buildingTypeID < buildingCatalogue.allBuildings.count)
	{
		BuildingDef *found = get(&buildingCatalogue.allBuildings, buildingTypeID);
		if (found != null) result = found;
	}

	return result;
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

inline bool buildingDefHasType(BuildingDef *def, s32 typeID)
{
	bool result = (def->typeID == typeID);

	if (def->isIntersection)
	{
		result = result || (def->intersectionPart1TypeID == typeID) || (def->intersectionPart2TypeID == typeID);
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
		hasNext(&it);
		next(&it))
	{
		BuildingDef *def = getValue(&it);

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

bool matchesVariant(BuildingDef *def, BuildingVariant *variant, BuildingDef **neighbourDefs)
{
	DEBUG_FUNCTION();

	bool result = true;

	for (s32 directionIndex = 0; directionIndex < ConnectionDirectionCount; directionIndex++)
	{
		bool matchedDirection = false;
		ConnectionType connectionType = variant->connections[directionIndex];
		BuildingDef *neighbourDef = neighbourDefs[directionIndex];

		if (neighbourDef == null)
		{
			matchedDirection = (connectionType == ConnectionType_Nothing) || (connectionType == ConnectionType_Anything);
		}
		else
		{
			switch (connectionType)
			{
				case ConnectionType_Nothing: {
					if (def->isIntersection)
					{
						matchedDirection = !buildingDefHasType(neighbourDef, def->intersectionPart1TypeID)
							  			&& !buildingDefHasType(neighbourDef, def->intersectionPart2TypeID);
					}
					else
					{
						matchedDirection = !buildingDefHasType(neighbourDef, def->typeID);
					}
				} break;

				case ConnectionType_Building1: {
					if (def->isIntersection)
					{
						matchedDirection = buildingDefHasType(neighbourDef, def->intersectionPart1TypeID);
					}
					else
					{
						matchedDirection = buildingDefHasType(neighbourDef, def->typeID);
					}
				} break;

				case ConnectionType_Building2: {
					if (def->isIntersection)
					{
						matchedDirection = buildingDefHasType(neighbourDef, def->intersectionPart2TypeID);
					}
					else
					{
						matchedDirection = false;
					}
				} break;

				case ConnectionType_Anything: {
					matchedDirection = true;
				} break;

				case ConnectionType_Invalid:
				default: {
					matchedDirection = false;
				} break;
			}
		}

		if (matchedDirection == false)
		{
			result = false;
			break;
		}
	}

	return result;
}

void updateBuildingVariant(City *city, Building *building, BuildingDef *passedDef)
{
	DEBUG_FUNCTION();

	if (building == null) return;

	BuildingDef *def = (passedDef != null) ? passedDef : getBuildingDef(building->typeID);

	if (def->variants.count > 0)
	{
		// NB: Right now we only allow variants for 1x1 buildings.
		// Later, we might want to expand that, but it will make things a LOT more complicated, so I'm
		// starting simple!
		// The only non-1x1 cases I can think of could be worked around, too, so it's fine.
		// (eg, A train station with included rail - only the RAILS need variants, the station doesn't!)
		// - Sam, 14/02/2020
		

		// Calculate our connections
		s32 x = building->footprint.x;
		s32 y = building->footprint.y;

		static_assert(ConnectionDirectionCount == 8, "updateBuildingVariant() assumes ConnectionDirectionCount == 8");
		BuildingDef *neighbourDefs[ConnectionDirectionCount] = {
			getBuildingDef(getBuildingAt(city, x    , y - 1)),
			getBuildingDef(getBuildingAt(city, x + 1, y - 1)),
			getBuildingDef(getBuildingAt(city, x + 1, y    )),
			getBuildingDef(getBuildingAt(city, x + 1, y + 1)),
			getBuildingDef(getBuildingAt(city, x    , y + 1)),
			getBuildingDef(getBuildingAt(city, x - 1, y + 1)),
			getBuildingDef(getBuildingAt(city, x - 1, y    )),
			getBuildingDef(getBuildingAt(city, x - 1, y - 1))
		};

		// Search for a matching variant
		// Right now... YAY LINEAR SEARCH! @Speed
		bool foundVariant = false;
		for (s32 variantIndex = 0; variantIndex < def->variants.count; variantIndex++)
		{
			BuildingVariant *variant = &def->variants[variantIndex];
			if (matchesVariant(def, variant, neighbourDefs))
			{
				building->variantIndex = variantIndex;
				foundVariant = true;
				logInfo("Matched building {0}#{1} with variant #{2}"_s, {def->name, formatInt(building->id), formatInt(variantIndex)});
				break;
			}
		}

		if (!foundVariant)
		{
			if (!isEmpty(def->spriteName))
			{
				logWarn("Unable to find a matching variant for building '{0}'. Defaulting to the building's defined sprite, '{1}'."_s, {def->name, def->spriteName});
				building->variantIndex = NO_VARIANT;
			}
			else
			{
				logWarn("Unable to find a matching variant for building '{0}'. Defaulting to variant #0."_s, {def->name});
				building->variantIndex = 0;
			}
		}
	}
	else
	{
		building->variantIndex = NO_VARIANT;
	}
}

void updateAdjacentBuildingVariants(City *city, Rect2I footprint)
{
	DEBUG_FUNCTION();

	for (s32 y = footprint.y;
		y < footprint.y + footprint.h;
		y++)
	{
		Building *buildingL = getBuildingAt(city, footprint.x - 1, y);
		if (buildingL)
		{
			BuildingDef *defL = getBuildingDef(buildingL->typeID);
			if (defL->variants.count > 0) updateBuildingVariant(city, buildingL, defL);
		}

		Building *buildingR = getBuildingAt(city, footprint.x + footprint.w, y);
		if (buildingR)
		{
			BuildingDef *defD = getBuildingDef(buildingR->typeID);
			if (defD->variants.count > 0) updateBuildingVariant(city, buildingR, defD);
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
			BuildingDef *defU = getBuildingDef(buildingU->typeID);
			if (defU->variants.count > 0) updateBuildingVariant(city, buildingU, defU);
		}
		if (buildingD)
		{
			BuildingDef *defR = getBuildingDef(buildingD->typeID);
			if (defR->variants.count > 0) updateBuildingVariant(city, buildingD, defR);
		}
	}
}

Maybe<BuildingDef *> findBuildingIntersection(BuildingDef *defA, BuildingDef *defB)
{
	DEBUG_FUNCTION();

	Maybe<BuildingDef *> result = makeFailure<BuildingDef *>();

	// It's horrible linear search time!
	for (auto it = iterate(&buildingCatalogue.intersectionBuildings); hasNext(&it); next(&it))
	{
		BuildingDef *itDef = getValue(&it);

		if (itDef->isIntersection)
		{
			if (((itDef->intersectionPart1TypeID == defA->typeID) && (itDef->intersectionPart2TypeID == defB->typeID))
				|| ((itDef->intersectionPart2TypeID == defA->typeID) && (itDef->intersectionPart1TypeID == defB->typeID)))
			{
				result = makeSuccess(itDef);
				break;
			}
		}
	}

	return result;
}

void refreshBuildingSpriteCache(BuildingCatalogue *catalogue)
{
	DEBUG_FUNCTION();

	for (auto it = iterate(&catalogue->allBuildings); hasNext(&it); next(&it))
	{
		BuildingDef *def = get(&it);

		// Account for the "null" building
		if (!isEmpty(def->spriteName))
		{
			def->sprites = getSpriteGroup(def->spriteName);
		}

		for (s32 variantIndex = 0; variantIndex < def->variants.count; variantIndex++)
		{
			def->variants[variantIndex].sprites = getSpriteGroup(def->variants[variantIndex].spriteName);
		}
	}
}

inline void addProblem(Building *building, BuildingProblem problem)
{
	building->problems += problem;
}

inline void removeProblem(Building *building, BuildingProblem problem)
{
	building->problems -= problem;
}

inline bool hasProblem(Building *building, BuildingProblem problem)
{
	return building->problems & problem;
}

inline Sprite *getBuildingSprite(Building *building)
{
	Sprite *result = null;

	BuildingDef *def = getBuildingDef(building->typeID);

	if (building->variantIndex != NO_VARIANT)
	{
		SpriteGroup *sprites = def->variants[building->variantIndex].sprites;
		if (sprites != null)
		{
			result = getSprite(sprites, building->spriteOffset);
		}
	}
	else
	{
		SpriteGroup *sprites = def->sprites;
		if (sprites != null)
		{
			result = getSprite(sprites, building->spriteOffset);
		}
	}

	return result;
}

inline ConnectionType connectionTypeOf(char c)
{
	ConnectionType result;

	switch (c)
	{
		case '0':  result = ConnectionType_Nothing;    break;
		case '1':  result = ConnectionType_Building1;  break;
		case '2':  result = ConnectionType_Building2;  break;
		case '*':  result = ConnectionType_Anything;   break;

		default:   result = ConnectionType_Invalid;    break;
	}

	return result;
}

inline char asChar(ConnectionType connectionType)
{
	char result;

	switch (connectionType)
	{
		case ConnectionType_Nothing:    result =  '0';  break;
		case ConnectionType_Building1:  result =  '1';  break;
		case ConnectionType_Building2:  result =  '2';  break;
		case ConnectionType_Anything:   result =  '*';  break;

		default:   result = '@';    break;
	}

	return result;
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

inline bool buildingHasPower(Building *building)
{
	return !(building->problems & BuildingProblem_NoPower);
}

void saveBuildingTypes()
{
	// Post-processing of BuildingDefs
	for (auto it = iterate(&buildingCatalogue.allBuildings); hasNext(&it); next(&it))
	{
		auto def = get(&it);
		if (def->isIntersection)
		{
			BuildingDef *part1Def = findBuildingDef(def->intersectionPart1Name);
			BuildingDef *part2Def = findBuildingDef(def->intersectionPart2Name);

			if (part1Def == null)
			{
				logError("Unable to find building named '{0}' for part 1 of intersection '{1}'."_s, {def->intersectionPart1Name, def->name});
				def->intersectionPart1TypeID = 0;
			}
			else
			{
				def->intersectionPart1TypeID = part1Def->typeID;
			}

			if (part2Def == null)
			{
				logError("Unable to find building named '{0}' for part 2 of intersection '{1}'."_s, {def->intersectionPart2Name, def->name});
				def->intersectionPart2TypeID = 0;
			}
			else
			{
				def->intersectionPart2TypeID = part2Def->typeID;
			}
		}
	}

	// Actual saving
	putAll(&buildingCatalogue.buildingNameToOldTypeID, &buildingCatalogue.buildingNameToTypeID);
}

void remapBuildingTypes(City *city)
{
	// First, remap any IDs that are not present in the current data, so they won't get
	// merged accidentally.
	for (auto it = iterate(&buildingCatalogue.buildingNameToOldTypeID); hasNext(&it); next(&it))
	{
		auto entry = getEntry(&it);
		if (!contains(&buildingCatalogue.buildingNameToTypeID, entry->key))
		{
			put(&buildingCatalogue.buildingNameToTypeID, entry->key, buildingCatalogue.buildingNameToTypeID.count);
		}
	}

	if (buildingCatalogue.buildingNameToOldTypeID.count > 0)
	{
		Array<s32> oldTypeToNewType = allocateArray<s32>(tempArena, buildingCatalogue.buildingNameToOldTypeID.count);
		for (auto it = iterate(&buildingCatalogue.buildingNameToOldTypeID); hasNext(&it); next(&it))
		{
			auto entry = getEntry(&it);
			String buildingName = entry->key;
			s32 oldType         = entry->value;

			s32 *newType = find(&buildingCatalogue.buildingNameToTypeID, buildingName);
			if (newType == null)
			{
				oldTypeToNewType[oldType] = 0;
			}
			else
			{
				oldTypeToNewType[oldType] = *newType;
			}
		}

		for (auto it = iterate(&city->buildings); hasNext(&it); next(&it))
		{
			Building *building = get(&it);
			s32 oldType = building->typeID;
			if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0))
			{
				building->typeID = oldTypeToNewType[oldType];
			}
		}
	}

	saveBuildingTypes();
}
