/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "building.h"
#include "AppState.h"
#include "city.h"
#include "line_reader.h"
#include <Assets/AssetManager.h>
#include <Gfx/Colour.h>
#include <Util/Deferred.h>

void initBuildingCatalogue()
{
    BuildingCatalogue* catalogue = &buildingCatalogue;
    *catalogue = {};

    auto& app_state = AppState::the();

    initChunkedArray(&catalogue->constructibleBuildings, &app_state.systemArena, 64);
    initChunkedArray(&catalogue->rGrowableBuildings, &app_state.systemArena, 64);
    initChunkedArray(&catalogue->cGrowableBuildings, &app_state.systemArena, 64);
    initChunkedArray(&catalogue->iGrowableBuildings, &app_state.systemArena, 64);
    initChunkedArray(&catalogue->intersectionBuildings, &app_state.systemArena, 64);

    initOccupancyArray(&catalogue->allBuildings, &app_state.systemArena, 64);
    // NB: BuildingDef ids are 1-indexed. At least one place (BuildingDef.canBeBuiltOnID) uses 0 as a "none" value.
    // So, we have to append a blank for a "null" def. Could probably get rid of it, but initialise-to-zero is convenient
    // and I'm likely to accidentally leave other things set to 0, so it's safer to just keep the null def.
    // Update 18/02/2020: We now use the null building def when failing to match an intersection part name.
    Indexed<BuildingDef*> nullBuildingDef = catalogue->allBuildings.append();
    *nullBuildingDef.value = {};

    initHashTable(&catalogue->buildingsByName, 0.75f, 128);
    initStringTable(&catalogue->buildingNames);

    initHashTable(&catalogue->buildingNameToTypeID, 0.75f, 128);
    catalogue->buildingNameToTypeID.put(nullString, 0);
    initHashTable(&catalogue->buildingNameToOldTypeID, 0.75f, 128);

    catalogue->maxRBuildingDim = 0;
    catalogue->maxCBuildingDim = 0;
    catalogue->maxIBuildingDim = 0;
    catalogue->overallMaxBuildingDim = 0;
}

void _assignBuildingCategories(BuildingCatalogue* catalogue, BuildingDef* def)
{
    if (def->typeID == 0)
        return; // Defs with typeID 0 are templates, which we don't want polluting the catalogue!

    catalogue->overallMaxBuildingDim = max(catalogue->overallMaxBuildingDim, max(def->width, def->height));

    if (def->buildMethod != BuildMethod::None) {
        catalogue->constructibleBuildings.append(def);
    }

    switch (def->growsInZone) {
    case ZoneType::Residential: {
        catalogue->rGrowableBuildings.append(def);
        catalogue->maxRBuildingDim = max(catalogue->maxRBuildingDim, max(def->width, def->height));
    } break;

    case ZoneType::Commercial: {
        catalogue->cGrowableBuildings.append(def);
        catalogue->maxCBuildingDim = max(catalogue->maxCBuildingDim, max(def->width, def->height));
    } break;

    case ZoneType::Industrial: {
        catalogue->iGrowableBuildings.append(def);
        catalogue->maxIBuildingDim = max(catalogue->maxIBuildingDim, max(def->width, def->height));
    } break;

    case ZoneType::None:
        break;

    default: {
        logDebug("Building {} has invalid growsInZone value ({})"_s, { def->name, formatInt(def->growsInZone) });
        ASSERT(false);
    } break;
    }

    if (def->isIntersection) {
        catalogue->intersectionBuildings.append(def);
    }

    ASSERT(catalogue->allBuildings.count == (catalogue->buildingsByName.count + 1)); // NB: +1 for the null building.
}

BuildingDef* appendNewBuildingDef(String name)
{
    Indexed<BuildingDef*> newDef = buildingCatalogue.allBuildings.append();
    BuildingDef* result = newDef.value;
    result->name = intern(&buildingCatalogue.buildingNames, name);
    result->typeID = newDef.index;

    result->fireRisk = 1.0f;
    buildingCatalogue.buildingsByName.put(result->name, result);
    buildingCatalogue.buildingNameToTypeID.put(result->name, result->typeID);

    return result;
}

void loadBuildingDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader = readLines(asset->shortName, data);

    BuildingCatalogue* catalogue = &buildingCatalogue;

    // Count the number of building defs in the file first, so we can allocate the buildingIDs array in the asset
    s32 buildingCount = 0;
    // Same for variants as they have their own structs
    s32 totalVariantCount = 0;
    while (loadNextLine(&reader)) {
        String command = readToken(&reader);
        if (equals(command, ":Building"_s) || equals(command, ":Intersection"_s)) {
            buildingCount++;
        } else if (equals(command, "variant"_s)) {
            totalVariantCount++;
        }
    }

    smm buildingNamesSize = sizeof(String) * buildingCount;
    smm variantsSize = sizeof(BuildingVariant) * totalVariantCount;
    asset->data = assetsAllocate(&asset_manager(), buildingNamesSize + variantsSize);
    asset->buildingDefs.buildingIDs = makeArray(buildingCount, (String*)asset->data.writable_data());
    u8* variantsMemory = asset->data.writable_data() + buildingNamesSize;

    restart(&reader);

    HashTable<BuildingDef> templates;
    initHashTable(&templates);
    Deferred defer_free_hash_table = [&templates] { freeHashTable(&templates); };

    BuildingDef* def = nullptr;

    while (loadNextLine(&reader)) {
        String firstWord = readToken(&reader);

        if (firstWord[0] == ':') // Definitions
        {
            // Define something
            firstWord.chars++;
            firstWord.length--;

            if (def != nullptr) {
                // Now that the previous building is done, we can categorise it
                _assignBuildingCategories(catalogue, def);
            }

            if (equals(firstWord, "Building"_s)) {
                String name = readToken(&reader);
                if (isEmpty(name)) {
                    error(&reader, "Couldn't parse Building. Expected: ':Building identifier'"_s);
                    return;
                }

                def = appendNewBuildingDef(name);
                asset->buildingDefs.buildingIDs.append(def->name);
            } else if (equals(firstWord, "Intersection"_s)) {
                String name = readToken(&reader);
                String part1Name = readToken(&reader);
                String part2Name = readToken(&reader);

                if (isEmpty(name) || isEmpty(part1Name) || isEmpty(part2Name)) {
                    error(&reader, "Couldn't parse Intersection. Expected: ':Intersection identifier part1 part2'"_s);
                    return;
                }

                def = appendNewBuildingDef(name);
                asset->buildingDefs.buildingIDs.append(def->name);

                def->isIntersection = true;
                def->intersectionPart1Name = intern(&catalogue->buildingNames, part1Name);
                def->intersectionPart2Name = intern(&catalogue->buildingNames, part2Name);
            } else if (equals(firstWord, "Template"_s)) {
                String name = readToken(&reader);
                if (isEmpty(name)) {
                    error(&reader, "Couldn't parse Template. Expected: ':Template identifier'"_s);
                    return;
                }

                def = templates.put(pushString(&temp_arena(), name));
            } else {
                warn(&reader, "Only :Building, :Intersection or :Template definitions are supported right now."_s);
            }

            // Read ahead to count how many variants this building/intersection has.
            s32 variantCount = countPropertyOccurrences(&reader, "variant"_s);
            if (variantCount > 0) {
                def->variants = makeArray(variantCount, (BuildingVariant*)variantsMemory);
                variantsMemory += sizeof(BuildingVariant) * variantCount;
            }

        } else // Properties!
        {
            if (def == nullptr) {
                error(&reader, "Found a property before starting a :Building, :Intersection or :Template!"_s);
                return;
            } else {
                if (equals(firstWord, "build"_s)) {
                    String buildMethodString = readToken(&reader);
                    Maybe<s32> cost = readInt<s32>(&reader);

                    if (cost.isValid) {
                        if (equals(buildMethodString, "paint"_s)) {
                            def->buildMethod = BuildMethod::Paint;
                        } else if (equals(buildMethodString, "plop"_s)) {
                            def->buildMethod = BuildMethod::Plop;
                        } else if (equals(buildMethodString, "line"_s)) {
                            def->buildMethod = BuildMethod::DragLine;
                        } else if (equals(buildMethodString, "rect"_s)) {
                            def->buildMethod = BuildMethod::DragRect;
                        } else {
                            warn(&reader, "Couldn't parse the build method, assuming NONE."_s);
                            def->buildMethod = BuildMethod::None;
                        }

                        def->buildCost = cost.value;
                    } else {
                        error(&reader, "Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all."_s);
                        return;
                    }
                } else if (equals(firstWord, "carries_power"_s)) {
                    Maybe<bool> boolRead = readBool(&reader);
                    if (boolRead.isValid) {
                        if (boolRead.value) {
                            def->flags.add(BuildingFlags::CarriesPower);
                        } else {
                            def->flags.remove(BuildingFlags::CarriesPower);
                        }
                    }
                } else if (equals(firstWord, "carries_transport"_s)) {
                    s32 tokenCount = countTokens(getRemainderOfLine(&reader));
                    for (s32 tokenIndex = 0; tokenIndex < tokenCount; tokenIndex++) {
                        String transportName = readToken(&reader);

                        if (equals(transportName, "road"_s)) {
                            def->transportTypes.add(TransportType::Road);
                        } else if (equals(transportName, "rail"_s)) {
                            def->transportTypes.add(TransportType::Rail);
                        } else {
                            warn(&reader, "Unrecognised transport type \"{0}\"."_s, { transportName });
                        }
                    }
                } else if (equals(firstWord, "crime_protection"_s)) {
                    Maybe<EffectRadius> crime_protection = readEffectRadius(&reader);
                    if (crime_protection.isValid) {
                        def->policeEffect = crime_protection.value;
                    }
                } else if (equals(firstWord, "demolish_cost"_s)) {
                    Maybe<s32> demolish_cost = readInt<s32>(&reader);
                    if (demolish_cost.isValid) {
                        def->demolishCost = demolish_cost.value;
                    }
                } else if (equals(firstWord, "extends"_s)) {
                    String templateName = readToken(&reader);

                    Maybe<BuildingDef*> templateDef = templates.find(templateName);
                    if (!templateDef.isValid) {
                        error(&reader, "Could not find template named '{0}'. Templates must be defined before the buildings that use them, and in the same file."_s, { templateName });
                    } else {
                        BuildingDef* tDef = templateDef.value;

                        // Copy the def... this could be messy
                        // (We can't just do copyMemory() because we don't want to change the name or typeID.)
                        def->flags = tDef->flags;
                        def->size = tDef->size;
                        def->spriteName = tDef->spriteName;
                        def->buildMethod = tDef->buildMethod;
                        def->buildCost = tDef->buildCost;
                        def->growsInZone = tDef->growsInZone;
                        def->demolishCost = tDef->demolishCost;
                        def->residents = tDef->residents;
                        def->jobs = tDef->jobs;
                        def->transportTypes = tDef->transportTypes;
                        def->power = tDef->power;
                        def->landValueEffect = tDef->landValueEffect;
                        def->pollutionEffect = tDef->pollutionEffect;
                        def->fireRisk = tDef->fireRisk;
                        def->fireProtection = tDef->fireProtection;
                    }
                } else if (equals(firstWord, "fire_protection"_s)) {
                    Maybe<EffectRadius> fire_protection = readEffectRadius(&reader);
                    if (fire_protection.isValid) {
                        def->fireProtection = fire_protection.value;
                    }
                } else if (equals(firstWord, "fire_risk"_s)) {
                    Maybe<double> fire_risk = readFloat(&reader);
                    if (fire_risk.isValid) {
                        def->fireRisk = (float)fire_risk.value;
                    }
                } else if (equals(firstWord, "grows_in"_s)) {
                    String zoneName = readToken(&reader);
                    if (equals(zoneName, "r"_s)) {
                        def->growsInZone = ZoneType::Residential;
                    } else if (equals(zoneName, "c"_s)) {
                        def->growsInZone = ZoneType::Commercial;
                    } else if (equals(zoneName, "i"_s)) {
                        def->growsInZone = ZoneType::Industrial;
                    } else {
                        error(&reader, "Couldn't parse grows_in. Expected use:\"grows_in r/c/i\""_s);
                        return;
                    }
                } else if (equals(firstWord, "health_effect"_s)) {
                    Maybe<EffectRadius> health_effect = readEffectRadius(&reader);
                    if (health_effect.isValid) {
                        def->healthEffect = health_effect.value;
                    }
                } else if (equals(firstWord, "jail_size"_s)) {
                    Maybe<s32> jail_size = readInt<s32>(&reader);
                    if (jail_size.isValid) {
                        def->jailCapacity = jail_size.value;
                    }
                } else if (equals(firstWord, "jobs"_s)) {
                    Maybe<s32> jobs = readInt<s32>(&reader);
                    if (jobs.isValid) {
                        def->jobs = jobs.value;
                    }
                } else if (equals(firstWord, "land_value"_s)) {
                    Maybe<EffectRadius> land_value = readEffectRadius(&reader);
                    if (land_value.isValid) {
                        def->landValueEffect = land_value.value;
                    }
                } else if (equals(firstWord, "name"_s)) {
                    def->textAssetName = intern(&asset_manager().assetStrings, readToken(&reader));
                } else if (equals(firstWord, "pollution"_s)) {
                    Maybe<EffectRadius> pollution = readEffectRadius(&reader);
                    if (pollution.isValid) {
                        def->pollutionEffect = pollution.value;
                    }
                } else if (equals(firstWord, "power_gen"_s)) {
                    Maybe<s32> power_gen = readInt<s32>(&reader);
                    if (power_gen.isValid) {
                        def->power = power_gen.value;
                    }
                } else if (equals(firstWord, "power_use"_s)) {
                    Maybe<s32> power_use = readInt<s32>(&reader);
                    if (power_use.isValid) {
                        def->power = -power_use.value;
                    }
                } else if (equals(firstWord, "requires_transport_connection"_s)) {
                    Maybe<bool> requires_transport_connection = readBool(&reader);
                    if (requires_transport_connection.isValid) {
                        if (requires_transport_connection.value) {
                            def->flags.add(BuildingFlags::RequiresTransportConnection);
                        } else {
                            def->flags.remove(BuildingFlags::RequiresTransportConnection);
                        }
                    }
                } else if (equals(firstWord, "residents"_s)) {
                    Maybe<s32> residents = readInt<s32>(&reader);
                    if (residents.isValid) {
                        def->residents = residents.value;
                    }
                } else if (equals(firstWord, "size"_s)) {
                    Maybe<s32> w = readInt<s32>(&reader);
                    Maybe<s32> h = readInt<s32>(&reader);

                    if (w.isValid && h.isValid) {
                        def->width = w.value;
                        def->height = h.value;

                        if ((def->variants.count > 0) && (def->width != 1 || def->height != 1)) {
                            error(&reader, "This building is {0}x{1} and has variants. Variants are only allowed for 1x1 tile buildings!"_s, { formatInt(def->width), formatInt(def->height) });
                            return;
                        }
                    } else {
                        error(&reader, "Couldn't parse size. Expected 2 ints (w,h)."_s);
                        return;
                    }
                } else if (equals(firstWord, "sprite"_s)) {
                    String spriteName = intern(&asset_manager().assetStrings, readToken(&reader));
                    def->spriteName = spriteName;
                } else if (equals(firstWord, "variant"_s)) {
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

                    if (def->variants.count < def->variants.capacity) {
                        BuildingVariant* variant = def->variants.append();

                        *variant = {};

                        String directionFlags = readToken(&reader);
                        String spriteName = intern(&asset_manager().assetStrings, readToken(&reader));

                        // Check the values are valid first, because that's less verbose than checking each one individually.
                        for (auto i = 0; i < directionFlags.length; i++) {
                            if (!connectionTypeOf(directionFlags[i]).has_value()) {
                                error(&reader, "Unrecognized connection type character '{0}', valid values: '012*'"_s, { repeatChar(directionFlags[i], 1) });
                            }
                        }

                        if (directionFlags.length == 8) {
                            variant->connections[ConnectionDirection::N] = connectionTypeOf(directionFlags[0]).value();
                            variant->connections[ConnectionDirection::NE] = connectionTypeOf(directionFlags[1]).value();
                            variant->connections[ConnectionDirection::E] = connectionTypeOf(directionFlags[2]).value();
                            variant->connections[ConnectionDirection::SE] = connectionTypeOf(directionFlags[3]).value();
                            variant->connections[ConnectionDirection::S] = connectionTypeOf(directionFlags[4]).value();
                            variant->connections[ConnectionDirection::SW] = connectionTypeOf(directionFlags[5]).value();
                            variant->connections[ConnectionDirection::W] = connectionTypeOf(directionFlags[6]).value();
                            variant->connections[ConnectionDirection::NW] = connectionTypeOf(directionFlags[7]).value();
                        } else if (directionFlags.length == 4) {
                            // The 4 other directions don't matter
                            variant->connections[ConnectionDirection::NE] = ConnectionType::Anything;
                            variant->connections[ConnectionDirection::SE] = ConnectionType::Anything;
                            variant->connections[ConnectionDirection::SW] = ConnectionType::Anything;
                            variant->connections[ConnectionDirection::NW] = ConnectionType::Anything;

                            variant->connections[ConnectionDirection::N] = connectionTypeOf(directionFlags[0]).value();
                            variant->connections[ConnectionDirection::E] = connectionTypeOf(directionFlags[1]).value();
                            variant->connections[ConnectionDirection::S] = connectionTypeOf(directionFlags[2]).value();
                            variant->connections[ConnectionDirection::W] = connectionTypeOf(directionFlags[3]).value();
                        } else {
                            error(&reader, "First argument for a building 'variant' should be a 4 or 8 character string consisting of 0/1/2/* flags (meaning nothing/part1/part2/anything) for N/E/S/W or N/NE/E/SE/S/SW/W/NW connectivity. eg, 101012**"_s);
                            return;
                        }

                        variant->spriteName = spriteName;
                    } else {
                        error(&reader, "Too many variants for building '{0}'!"_s, { def->name });
                    }
                } else {
                    error(&reader, "Unrecognized token: {0}"_s, { firstWord });
                }
            }
        }
    }

    if (def != nullptr) {
        // Categorise the last building
        _assignBuildingCategories(catalogue, def);
    }

    logInfo("Loaded {0} buildings: R:{1} C:{2} I:{3} growable, player-constructible:{4}, intersections:{5}"_s, { formatInt(catalogue->allBuildings.count), formatInt(catalogue->rGrowableBuildings.count), formatInt(catalogue->cGrowableBuildings.count), formatInt(catalogue->iGrowableBuildings.count), formatInt(catalogue->constructibleBuildings.count), formatInt(catalogue->intersectionBuildings.count) });
}

void removeBuildingDefs(Array<String> idsToRemove)
{
    BuildingCatalogue* catalogue = &buildingCatalogue;

    for (s32 idIndex = 0; idIndex < idsToRemove.count; idIndex++) {
        String buildingID = idsToRemove[idIndex];
        BuildingDef* def = findBuildingDef(buildingID);
        if (def != nullptr) {
            catalogue->constructibleBuildings.findAndRemove(def);
            catalogue->rGrowableBuildings.findAndRemove(def);
            catalogue->cGrowableBuildings.findAndRemove(def);
            catalogue->iGrowableBuildings.findAndRemove(def);
            catalogue->intersectionBuildings.findAndRemove(def);

            catalogue->buildingsByName.removeKey(buildingID);

            catalogue->allBuildings.removeIndex(def->typeID);

            catalogue->buildingNameToTypeID.removeKey(buildingID);
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

BuildingDef* getBuildingDef(s32 buildingTypeID)
{
    BuildingDef* result = buildingCatalogue.allBuildings.get(0);

    if (buildingTypeID > 0 && buildingTypeID < buildingCatalogue.allBuildings.count) {
        BuildingDef* found = buildingCatalogue.allBuildings.get(buildingTypeID);
        if (found != nullptr)
            result = found;
    }

    return result;
}

BuildingDef* getBuildingDef(Building* building)
{
    BuildingDef* result = nullptr;

    if (building != nullptr) {
        result = getBuildingDef(building->typeID);
    }

    return result;
}

BuildingDef* findBuildingDef(String name)
{
    BuildingDef* result = buildingCatalogue.buildingsByName.findValue(name).orDefault(nullptr);

    return result;
}

bool buildingDefHasType(BuildingDef* def, s32 typeID)
{
    bool result = (def->typeID == typeID);

    if (def->isIntersection) {
        result = result || (def->intersectionPart1TypeID == typeID) || (def->intersectionPart2TypeID == typeID);
    }

    return result;
}

ChunkedArray<BuildingDef*>* getConstructibleBuildings()
{
    return &buildingCatalogue.constructibleBuildings;
}

s32 getMaxBuildingSize(ZoneType zoneType)
{
    s32 result = 0;

    switch (zoneType) {
    case ZoneType::Residential:
        result = buildingCatalogue.maxRBuildingDim;
        break;
    case ZoneType::Commercial:
        result = buildingCatalogue.maxCBuildingDim;
        break;
    case ZoneType::Industrial:
        result = buildingCatalogue.maxIBuildingDim;
        break;

        INVALID_DEFAULT_CASE;
    }

    return result;
}

bool matchesVariant(BuildingDef* def, BuildingVariant* variant, EnumMap<ConnectionDirection, BuildingDef*> const& neighbourDefs)
{
    DEBUG_FUNCTION();

    bool result = true;

    for (auto direction : enum_values<ConnectionDirection>()) {
        bool matchedDirection = false;
        ConnectionType connectionType = variant->connections[direction];
        BuildingDef* neighbourDef = neighbourDefs[direction];

        if (neighbourDef == nullptr) {
            matchedDirection = (connectionType == ConnectionType::Nothing) || (connectionType == ConnectionType::Anything);
        } else {
            switch (connectionType) {
            case ConnectionType::Nothing: {
                if (def->isIntersection) {
                    matchedDirection = !buildingDefHasType(neighbourDef, def->intersectionPart1TypeID)
                        && !buildingDefHasType(neighbourDef, def->intersectionPart2TypeID);
                } else {
                    matchedDirection = !buildingDefHasType(neighbourDef, def->typeID);
                }
            } break;

            case ConnectionType::Building1: {
                if (def->isIntersection) {
                    matchedDirection = buildingDefHasType(neighbourDef, def->intersectionPart1TypeID);
                } else {
                    matchedDirection = buildingDefHasType(neighbourDef, def->typeID);
                }
            } break;

            case ConnectionType::Building2: {
                if (def->isIntersection) {
                    matchedDirection = buildingDefHasType(neighbourDef, def->intersectionPart2TypeID);
                } else {
                    matchedDirection = false;
                }
            } break;

            case ConnectionType::Anything: {
                matchedDirection = true;
            } break;

            default: {
                matchedDirection = false;
            } break;
            }
        }

        if (matchedDirection == false) {
            result = false;
            break;
        }
    }

    return result;
}

void updateBuildingVariant(City* city, Building* building, BuildingDef* passedDef)
{
    DEBUG_FUNCTION();

    if (building == nullptr)
        return;

    BuildingDef* def = (passedDef != nullptr) ? passedDef : getBuildingDef(building->typeID);

    if (def->variants.count > 0) {
        // NB: Right now we only allow variants for 1x1 buildings.
        // Later, we might want to expand that, but it will make things a LOT more complicated, so I'm
        // starting simple!
        // The only non-1x1 cases I can think of could be worked around, too, so it's fine.
        // (eg, A train station with included rail - only the RAILS need variants, the station doesn't!)
        // - Sam, 14/02/2020

        // Calculate our connections
        s32 x = building->footprint.x;
        s32 y = building->footprint.y;

        static_assert(to_underlying(ConnectionDirection::COUNT) == 8, "updateBuildingVariant() assumes ConnectionDirectionCount == 8");
        EnumMap<ConnectionDirection, BuildingDef*> neighbourDefs;
        for (auto direction : enum_values<ConnectionDirection>()) {
            neighbourDefs[direction] = getBuildingDef(getBuildingAt(city, x + connection_offsets[direction].x, y + connection_offsets[direction].y));
        }

        // Search for a matching variant
        // Right now... YAY LINEAR SEARCH! @Speed
        bool foundVariant = false;
        for (s32 variantIndex = 0; variantIndex < def->variants.count; variantIndex++) {
            BuildingVariant* variant = &def->variants[variantIndex];
            if (matchesVariant(def, variant, neighbourDefs)) {
                building->variantIndex = variantIndex;
                foundVariant = true;
                logInfo("Matched building {0}#{1} with variant #{2}"_s, { def->name, formatInt(building->id), formatInt(variantIndex) });
                break;
            }
        }

        if (!foundVariant) {
            if (!isEmpty(def->spriteName)) {
                logWarn("Unable to find a matching variant for building '{0}'. Defaulting to the building's defined sprite, '{1}'."_s, { def->name, def->spriteName });
                building->variantIndex = {};
            } else {
                logWarn("Unable to find a matching variant for building '{0}'. Defaulting to variant #0."_s, { def->name });
                building->variantIndex = 0;
            }
        }
    } else {
        building->variantIndex = {};
    }

    // Update the entity sprite
    loadBuildingSprite(building);
}

void updateAdjacentBuildingVariants(City* city, Rect2I footprint)
{
    DEBUG_FUNCTION();

    for (s32 y = footprint.y;
        y < footprint.y + footprint.h;
        y++) {
        Building* buildingL = getBuildingAt(city, footprint.x - 1, y);
        if (buildingL) {
            BuildingDef* defL = getBuildingDef(buildingL->typeID);
            if (defL->variants.count > 0)
                updateBuildingVariant(city, buildingL, defL);
        }

        Building* buildingR = getBuildingAt(city, footprint.x + footprint.w, y);
        if (buildingR) {
            BuildingDef* defD = getBuildingDef(buildingR->typeID);
            if (defD->variants.count > 0)
                updateBuildingVariant(city, buildingR, defD);
        }
    }

    for (s32 x = footprint.x;
        x < footprint.x + footprint.w;
        x++) {
        Building* buildingU = getBuildingAt(city, x, footprint.y - 1);
        Building* buildingD = getBuildingAt(city, x, footprint.y + footprint.h);
        if (buildingU) {
            BuildingDef* defU = getBuildingDef(buildingU->typeID);
            if (defU->variants.count > 0)
                updateBuildingVariant(city, buildingU, defU);
        }
        if (buildingD) {
            BuildingDef* defR = getBuildingDef(buildingD->typeID);
            if (defR->variants.count > 0)
                updateBuildingVariant(city, buildingD, defR);
        }
    }
}

Maybe<BuildingDef*> findBuildingIntersection(BuildingDef* defA, BuildingDef* defB)
{
    DEBUG_FUNCTION();

    Maybe<BuildingDef*> result = makeFailure<BuildingDef*>();

    // It's horrible linear search time!
    for (auto it = buildingCatalogue.intersectionBuildings.iterate(); it.hasNext(); it.next()) {
        BuildingDef* itDef = it.getValue();

        if (itDef->isIntersection) {
            if (((itDef->intersectionPart1TypeID == defA->typeID) && (itDef->intersectionPart2TypeID == defB->typeID))
                || ((itDef->intersectionPart2TypeID == defA->typeID) && (itDef->intersectionPart1TypeID == defB->typeID))) {
                result = makeSuccess(itDef);
                break;
            }
        }
    }

    return result;
}

void initBuilding(Building* building, s32 id, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate)
{
    *building = {};

    building->id = id;
    building->typeID = def->typeID;
    building->creationDate = creationDate;
    building->footprint = footprint;
    building->variantIndex = {};
}

void updateBuilding(City* city, Building* building)
{
    BuildingDef* def = getBuildingDef(building->typeID);

    // Check the building's needs are met
    // ... except for the ones that are checked by layers.

    // Distance to road
    // TODO: Replace with access to any transport types, instead of just road? Not sure what we want with that.
    if ((def->flags.has(BuildingFlags::RequiresTransportConnection)) || (def->growsInZone != ZoneType::None)) {
        s32 distanceToRoad = s32Max;
        // TODO: @Speed: We only actually need to check the boundary tiles, because they're guaranteed to be less than
        // the inner tiles... unless we allow multiple buildings per tile. Actually maybe we do? I'm not sure how that
        // would work really. Anyway, can think about that later.
        // - Sam, 30/08/2019
        for (s32 y = building->footprint.y; y < building->footprint.y + building->footprint.h; y++) {
            for (s32 x = building->footprint.x; x < building->footprint.x + building->footprint.w; x++) {
                distanceToRoad = min(distanceToRoad, getDistanceToTransport(city, x, y, TransportType::Road));
            }
        }

        if (def->growsInZone != ZoneType::None) {
            // Zoned buildings inherit their zone's max distance to road.
            if (distanceToRoad > ZONE_DEFS[def->growsInZone].maximumDistanceToRoad) {
                addProblem(building, BuildingProblem::Type::NoTransportAccess);
            } else {
                removeProblem(building, BuildingProblem::Type::NoTransportAccess);
            }
        } else if (def->flags.has(BuildingFlags::RequiresTransportConnection)) {
            // Other buildings require direct contact
            if (distanceToRoad > 1) {
                addProblem(building, BuildingProblem::Type::NoTransportAccess);
            } else {
                removeProblem(building, BuildingProblem::Type::NoTransportAccess);
            }
        }
    }

    // Fire!
    if (doesAreaContainFire(city, building->footprint)) {
        addProblem(building, BuildingProblem::Type::Fire);
    } else {
        removeProblem(building, BuildingProblem::Type::Fire);
    }

    // Power!
    if (def->power < 0) {
        if (-def->power > building->allocatedPower) {
            addProblem(building, BuildingProblem::Type::NoPower);
        } else {
            removeProblem(building, BuildingProblem::Type::NoPower);
        }
    }

    // Now, colour the building based on its problems
    auto drawColorNormal = Colour::white();
    auto drawColorNoPower = Colour::from_rgb_255(32, 32, 64, 255);

    if (!buildingHasPower(building)) {
        building->entity->color = drawColorNoPower;
    } else {
        building->entity->color = drawColorNormal;
    }
}

void addProblem(Building* building, BuildingProblem::Type problem)
{
    BuildingProblem* bp = &building->problems[problem];
    if (!bp->isActive) {
        bp->isActive = true;
        bp->type = problem;
        bp->startDate = getCurrentTimestamp();
    }

    // TODO: Update zots!
}

void removeProblem(Building* building, BuildingProblem::Type problem)
{
    if (building->problems[problem].isActive) {
        building->problems[problem].isActive = false;

        // TODO: Update zots!
    }
}

bool hasProblem(Building* building, BuildingProblem::Type problem)
{
    bool result = building->problems[problem].isActive;

    return result;
}

void loadBuildingSprite(Building* building)
{
    BuildingDef* def = getBuildingDef(building->typeID);

    if (building->variantIndex.has_value()) {
        building->entity->sprite = getSpriteRef(def->variants[building->variantIndex.value()].spriteName, building->spriteOffset);
    } else {
        building->entity->sprite = getSpriteRef(def->spriteName, building->spriteOffset);
    }
}

Optional<ConnectionType> connectionTypeOf(char c)
{
    switch (c) {
    case '0':
        return ConnectionType::Nothing;
    case '1':
        return ConnectionType::Building1;
    case '2':
        return ConnectionType::Building2;
    case '*':
        return ConnectionType::Anything;
    default:
        return {};
    }
}

char asChar(ConnectionType connectionType)
{
    char result;

    switch (connectionType) {
    case ConnectionType::Nothing:
        result = '0';
        break;
    case ConnectionType::Building1:
        result = '1';
        break;
    case ConnectionType::Building2:
        result = '2';
        break;
    case ConnectionType::Anything:
        result = '*';
        break;

    default:
        result = '?';
        break;
    }

    return result;
}

s32 getRequiredPower(Building* building)
{
    s32 result = 0;

    BuildingDef* def = getBuildingDef(building->typeID);
    if (def->power < 0) {
        result = -def->power;
    }

    return result;
}

bool buildingHasPower(Building* building)
{
    return !hasProblem(building, BuildingProblem::Type::NoPower);
}

void saveBuildingTypes()
{
    // Post-processing of BuildingDefs
    for (auto it = buildingCatalogue.allBuildings.iterate(); it.hasNext(); it.next()) {
        auto def = it.get();
        if (def->isIntersection) {
            BuildingDef* part1Def = findBuildingDef(def->intersectionPart1Name);
            BuildingDef* part2Def = findBuildingDef(def->intersectionPart2Name);

            if (part1Def == nullptr) {
                logError("Unable to find building named '{0}' for part 1 of intersection '{1}'."_s, { def->intersectionPart1Name, def->name });
                def->intersectionPart1TypeID = 0;
            } else {
                def->intersectionPart1TypeID = part1Def->typeID;
            }

            if (part2Def == nullptr) {
                logError("Unable to find building named '{0}' for part 2 of intersection '{1}'."_s, { def->intersectionPart2Name, def->name });
                def->intersectionPart2TypeID = 0;
            } else {
                def->intersectionPart2TypeID = part2Def->typeID;
            }
        }
    }

    // Actual saving
    buildingCatalogue.buildingNameToOldTypeID.putAll(&buildingCatalogue.buildingNameToTypeID);
}

void remapBuildingTypes(City* city)
{
    // First, remap any IDs that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = buildingCatalogue.buildingNameToOldTypeID.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        if (!buildingCatalogue.buildingNameToTypeID.contains(entry->key)) {
            buildingCatalogue.buildingNameToTypeID.put(entry->key, buildingCatalogue.buildingNameToTypeID.count);
        }
    }

    if (buildingCatalogue.buildingNameToOldTypeID.count > 0) {
        Array<s32> oldTypeToNewType = temp_arena().allocate_array<s32>(buildingCatalogue.buildingNameToOldTypeID.count, true);
        for (auto it = buildingCatalogue.buildingNameToOldTypeID.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String buildingName = entry->key;
            s32 oldType = entry->value;

            oldTypeToNewType[oldType] = buildingCatalogue.buildingNameToTypeID.findValue(buildingName).orDefault(0);
        }

        for (auto it = city->buildings.iterate(); it.hasNext(); it.next()) {
            Building* building = it.get();
            s32 oldType = building->typeID;
            if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0)) {
                building->typeID = oldTypeToNewType[oldType];
            }
        }
    }

    saveBuildingTypes();
}
