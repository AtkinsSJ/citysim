/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BuildingCatalogue.h"

#include "AppState.h"
#include "Building.h"
#include "Util/Deferred.h"

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
    catalogue->allBuildings.append(); // Null building def

    initHashTable(&catalogue->buildingsByName, 0.75f, 128);
    initStringTable(&catalogue->buildingNames);

    initHashTable(&catalogue->buildingNameToTypeID, 0.75f, 128);
    catalogue->buildingNameToTypeID.put(nullString, 0);
    initHashTable(&catalogue->buildingNameToOldTypeID, 0.75f, 128);

    catalogue->maxRBuildingDim = 0;
    catalogue->maxCBuildingDim = 0;
    catalogue->maxIBuildingDim = 0;
    catalogue->overallMaxBuildingDim = 0;

    asset_manager().register_listener(catalogue);
}

void BuildingCatalogue::after_assets_loaded()
{
    remapBuildingTypes();
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
    Indexed<BuildingDef> newDef = buildingCatalogue.allBuildings.append();
    BuildingDef& result = newDef.value();
    result.name = intern(&buildingCatalogue.buildingNames, name);
    result.typeID = newDef.index();

    result.fireRisk = 1.0f;
    buildingCatalogue.buildingsByName.put(result.name, &result);
    buildingCatalogue.buildingNameToTypeID.put(result.name, result.typeID);

    return &result;
}

void loadBuildingDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader { asset->shortName, data };

    BuildingCatalogue* catalogue = &buildingCatalogue;

    // Count the number of building defs in the file first, so we can allocate the buildingIDs array in the asset
    s32 buildingCount = 0;
    // Same for variants as they have their own structs
    s32 totalVariantCount = 0;
    while (reader.load_next_line()) {
        String command = reader.next_token();
        if (command == ":Building"_s || command == ":Intersection"_s) {
            buildingCount++;
        } else if (command == "variant"_s) {
            totalVariantCount++;
        }
    }

    smm buildingNamesSize = sizeof(String) * buildingCount;
    smm variantsSize = sizeof(BuildingVariant) * totalVariantCount;
    asset->data = assetsAllocate(&asset_manager(), buildingNamesSize + variantsSize);
    asset->buildingDefs.buildingIDs = makeArray(buildingCount, (String*)asset->data.writable_data());
    u8* variantsMemory = asset->data.writable_data() + buildingNamesSize;

    reader.restart();

    HashTable<BuildingDef> templates;
    initHashTable(&templates);
    Deferred defer_free_hash_table = [&templates] { freeHashTable(&templates); };

    BuildingDef* def = nullptr;

    while (reader.load_next_line()) {
        String firstWord = reader.next_token();

        if (firstWord[0] == ':') // Definitions
        {
            // Define something
            firstWord.chars++;
            firstWord.length--;

            if (def != nullptr) {
                // Now that the previous building is done, we can categorise it
                _assignBuildingCategories(catalogue, def);
            }

            if (firstWord == "Building"_s) {
                String name = reader.next_token();
                if (name.is_empty()) {
                    reader.error("Couldn't parse Building. Expected: ':Building identifier'"_s);
                    return;
                }

                def = appendNewBuildingDef(name);
                asset->buildingDefs.buildingIDs.append(def->name);
            } else if (firstWord == "Intersection"_s) {
                String name = reader.next_token();
                String part1Name = reader.next_token();
                String part2Name = reader.next_token();

                if (name.is_empty() || part1Name.is_empty() || part2Name.is_empty()) {
                    reader.error("Couldn't parse Intersection. Expected: ':Intersection identifier part1 part2'"_s);
                    return;
                }

                def = appendNewBuildingDef(name);
                asset->buildingDefs.buildingIDs.append(def->name);

                def->isIntersection = true;
                def->intersectionPart1Name = intern(&catalogue->buildingNames, part1Name);
                def->intersectionPart2Name = intern(&catalogue->buildingNames, part2Name);
            } else if (firstWord == "Template"_s) {
                String name = reader.next_token();
                if (name.is_empty()) {
                    reader.error("Couldn't parse Template. Expected: ':Template identifier'"_s);
                    return;
                }

                def = templates.put(pushString(&temp_arena(), name));
            } else {
                reader.warn("Only :Building, :Intersection or :Template definitions are supported right now."_s);
            }

            // Read ahead to count how many variants this building/intersection has.
            s32 variantCount = reader.count_occurrences_of_property_in_current_command("variant"_s);
            if (variantCount > 0) {
                def->variants = makeArray(variantCount, (BuildingVariant*)variantsMemory);
                variantsMemory += sizeof(BuildingVariant) * variantCount;
            }

        } else // Properties!
        {
            if (def == nullptr) {
                reader.error("Found a property before starting a :Building, :Intersection or :Template!"_s);
                return;
            } else {
                if (firstWord == "build"_s) {
                    String buildMethodString = reader.next_token();
                    if (auto cost = reader.read_int<s32>(); cost.has_value()) {
                        if (buildMethodString == "paint"_s) {
                            def->buildMethod = BuildMethod::Paint;
                        } else if (buildMethodString == "plop"_s) {
                            def->buildMethod = BuildMethod::Plop;
                        } else if (buildMethodString == "line"_s) {
                            def->buildMethod = BuildMethod::DragLine;
                        } else if (buildMethodString == "rect"_s) {
                            def->buildMethod = BuildMethod::DragRect;
                        } else {
                            reader.warn("Couldn't parse the build method, assuming NONE."_s);
                            def->buildMethod = BuildMethod::None;
                        }

                        def->buildCost = cost.release_value();
                    } else {
                        reader.error("Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all."_s);
                        return;
                    }
                } else if (firstWord == "carries_power"_s) {
                    if (auto carries_power = reader.read_bool(); carries_power.has_value()) {
                        if (carries_power.value()) {
                            def->flags.add(BuildingFlags::CarriesPower);
                        } else {
                            def->flags.remove(BuildingFlags::CarriesPower);
                        }
                    }
                } else if (firstWord == "carries_transport"_s) {
                    s32 tokenCount = countTokens(reader.remainder_of_current_line());
                    for (s32 tokenIndex = 0; tokenIndex < tokenCount; tokenIndex++) {
                        String transportName = reader.next_token();

                        if (transportName == "road"_s) {
                            def->transportTypes.add(TransportType::Road);
                        } else if (transportName == "rail"_s) {
                            def->transportTypes.add(TransportType::Rail);
                        } else {
                            reader.warn("Unrecognised transport type \"{0}\"."_s, { transportName });
                        }
                    }
                } else if (firstWord == "crime_protection"_s) {
                    if (auto crime_protection = EffectRadius::read(reader); crime_protection.has_value()) {
                        def->policeEffect = crime_protection.release_value();
                    }
                } else if (firstWord == "demolish_cost"_s) {
                    if (auto demolish_cost = reader.read_int<s32>(); demolish_cost.has_value()) {
                        def->demolishCost = demolish_cost.release_value();
                    }
                } else if (firstWord == "extends"_s) {
                    String templateName = reader.next_token();

                    Maybe<BuildingDef*> templateDef = templates.find(templateName);
                    if (!templateDef.isValid) {
                        reader.error("Could not find template named '{0}'. Templates must be defined before the buildings that use them, and in the same file."_s, { templateName });
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
                } else if (firstWord == "fire_protection"_s) {
                    if (auto fire_protection = EffectRadius::read(reader); fire_protection.has_value()) {
                        def->fireProtection = fire_protection.release_value();
                    }
                } else if (firstWord == "fire_risk"_s) {
                    if (auto fire_risk = reader.read_float(); fire_risk.has_value()) {
                        def->fireRisk = fire_risk.release_value();
                    }
                } else if (firstWord == "grows_in"_s) {
                    String zoneName = reader.next_token();
                    if (zoneName == "r"_s) {
                        def->growsInZone = ZoneType::Residential;
                    } else if (zoneName == "c"_s) {
                        def->growsInZone = ZoneType::Commercial;
                    } else if (zoneName == "i"_s) {
                        def->growsInZone = ZoneType::Industrial;
                    } else {
                        reader.error("Couldn't parse grows_in. Expected use:\"grows_in r/c/i\""_s);
                        return;
                    }
                } else if (firstWord == "health_effect"_s) {
                    if (auto health_effect = EffectRadius::read(reader); health_effect.has_value()) {
                        def->healthEffect = health_effect.release_value();
                    }
                } else if (firstWord == "jail_size"_s) {
                    if (auto jail_size = reader.read_int<s32>(); jail_size.has_value()) {
                        def->jailCapacity = jail_size.release_value();
                    }
                } else if (firstWord == "jobs"_s) {
                    if (auto jobs = reader.read_int<s32>(); jobs.has_value()) {
                        def->jobs = jobs.release_value();
                    }
                } else if (firstWord == "land_value"_s) {
                    if (auto land_value = EffectRadius::read(reader); land_value.has_value()) {
                        def->landValueEffect = land_value.release_value();
                    }
                } else if (firstWord == "name"_s) {
                    def->textAssetName = intern(&asset_manager().assetStrings, reader.next_token());
                } else if (firstWord == "pollution"_s) {
                    if (auto pollution = EffectRadius::read(reader); pollution.has_value()) {
                        def->pollutionEffect = pollution.release_value();
                    }
                } else if (firstWord == "power_gen"_s) {
                    if (auto power_gen = reader.read_int<s32>(); power_gen.has_value()) {
                        def->power = power_gen.release_value();
                    }
                } else if (firstWord == "power_use"_s) {
                    if (auto power_use = reader.read_int<s32>(); power_use.has_value()) {
                        def->power = -power_use.release_value();
                    }
                } else if (firstWord == "requires_transport_connection"_s) {
                    if (auto requires_transport_connection = reader.read_bool(); requires_transport_connection.has_value()) {
                        if (requires_transport_connection.value()) {
                            def->flags.add(BuildingFlags::RequiresTransportConnection);
                        } else {
                            def->flags.remove(BuildingFlags::RequiresTransportConnection);
                        }
                    }
                } else if (firstWord == "residents"_s) {
                    if (auto residents = reader.read_int<s32>(); residents.has_value()) {
                        def->residents = residents.release_value();
                    }
                } else if (firstWord == "size"_s) {
                    auto w = reader.read_int<s32>();
                    auto h = reader.read_int<s32>();

                    if (w.has_value() && h.has_value()) {
                        def->width = w.release_value();
                        def->height = h.release_value();

                        if ((def->variants.count > 0) && (def->width != 1 || def->height != 1)) {
                            reader.error("This building is {0}x{1} and has variants. Variants are only allowed for 1x1 tile buildings!"_s, { formatInt(def->width), formatInt(def->height) });
                            return;
                        }
                    } else {
                        reader.error("Couldn't parse size. Expected 2 ints (w,h)."_s);
                        return;
                    }
                } else if (firstWord == "sprite"_s) {
                    String spriteName = intern(&asset_manager().assetStrings, reader.next_token());
                    def->spriteName = spriteName;
                } else if (firstWord == "variant"_s) {
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

                        String directionFlags = reader.next_token();
                        String spriteName = intern(&asset_manager().assetStrings, reader.next_token());

                        // Check the values are valid first, because that's less verbose than checking each one individually.
                        for (auto i = 0; i < directionFlags.length; i++) {
                            if (!connectionTypeOf(directionFlags[i]).has_value()) {
                                reader.error("Unrecognized connection type character '{0}', valid values: '012*'"_s, { repeatChar(directionFlags[i], 1) });
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
                            reader.error("First argument for a building 'variant' should be a 4 or 8 character string consisting of 0/1/2/* flags (meaning nothing/part1/part2/anything) for N/E/S/W or N/NE/E/SE/S/SW/W/NW connectivity. eg, 101012**"_s);
                            return;
                        }

                        variant->spriteName = spriteName;
                    } else {
                        reader.error("Too many variants for building '{0}'!"_s, { def->name });
                    }
                } else {
                    reader.error("Unrecognized token: {0}"_s, { firstWord });
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

BuildingDef* findBuildingDef(String name)
{
    BuildingDef* result = buildingCatalogue.buildingsByName.findValue(name).orDefault(nullptr);

    return result;
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

void remapBuildingTypes()
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

        auto& city = AppState::the().gameState->city;
        for (auto it = city.buildings.iterate(); it.hasNext(); it.next()) {
            Building* building = it.get();
            s32 oldType = building->typeID;
            if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0)) {
                building->typeID = oldTypeToNewType[oldType];
            }
        }
    }

    saveBuildingTypes();
}
