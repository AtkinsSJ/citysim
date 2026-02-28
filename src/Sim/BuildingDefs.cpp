/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BuildingDefs.h"
#include <Assets/AssetManager.h>
#include <Sim/Building.h>
#include <Sim/BuildingCatalogue.h>
#include <Util/Log.h>

static void assign_building_categories(BuildingCatalogue& catalogue, BuildingDef& def)
{
    if (def.typeID == 0)
        return; // Defs with typeID 0 are templates, which we don't want polluting the catalogue!

    catalogue.overallMaxBuildingDim = max(catalogue.overallMaxBuildingDim, max(def.size.x, def.size.y));

    if (def.buildMethod != BuildMethod::None) {
        catalogue.constructibleBuildings.append(&def);
    }

    switch (def.growsInZone) {
    case ZoneType::Residential: {
        catalogue.rGrowableBuildings.append(&def);
        catalogue.maxRBuildingDim = max(catalogue.maxRBuildingDim, max(def.size.x, def.size.y));
    } break;

    case ZoneType::Commercial: {
        catalogue.cGrowableBuildings.append(&def);
        catalogue.maxCBuildingDim = max(catalogue.maxCBuildingDim, max(def.size.x, def.size.y));
    } break;

    case ZoneType::Industrial: {
        catalogue.iGrowableBuildings.append(&def);
        catalogue.maxIBuildingDim = max(catalogue.maxIBuildingDim, max(def.size.x, def.size.y));
    } break;

    case ZoneType::None:
        break;

    default: {
        logDebug("Building {} has invalid growsInZone value ({})"_s, { def.name, formatInt(def.growsInZone) });
        ASSERT(false);
    } break;
    }

    if (def.isIntersection) {
        catalogue.intersectionBuildings.append(&def);
    }

    ASSERT(catalogue.allBuildings.count == catalogue.buildingsByName.count() + 1); // NB: +1 for the null building.
}

ErrorOr<NonnullOwnPtr<BuildingDefs>> BuildingDefs::load(AssetMetadata& metadata, Blob file_data)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, file_data };

    BuildingCatalogue* catalogue = &BuildingCatalogue::the();

    // Count the number of building defs in the file first, so we can allocate the building_ids array in the asset
    s32 buildingCount = 0;
    // Same for variants as they have their own structs
    s32 totalVariantCount = 0;
    while (reader.load_next_line()) {
        auto command = reader.next_token();
        if (command == ":Building"_s || command == ":Intersection"_s) {
            buildingCount++;
        } else if (command == "variant"_s) {
            totalVariantCount++;
        }
    }

    smm buildingNamesSize = sizeof(String) * buildingCount;
    smm variantsSize = sizeof(BuildingVariant) * totalVariantCount;
    auto data = Assets::assets_allocate(buildingNamesSize + variantsSize);
    auto building_ids = makeArray(buildingCount, reinterpret_cast<String*>(data.writable_data()));
    u8* variantsMemory = data.writable_data() + buildingNamesSize;

    reader.restart();

    HashTable<BuildingDef> templates;

    BuildingDef* def = nullptr;

    while (reader.load_next_line()) {
        auto maybe_first_word = reader.next_token();
        if (!maybe_first_word.has_value())
            break;
        auto& firstWord = maybe_first_word.value();

        // Definitions
        if (firstWord.starts_with(':')) {
            // Define something
            firstWord = firstWord.substring(1);

            if (def != nullptr) {
                // Now that the previous building is done, we can categorise it
                assign_building_categories(*catalogue, *def);
            }

            if (firstWord == "Building"_s) {
                auto name = reader.next_token();
                if (!name.has_value())
                    return reader.make_error_message("Couldn't parse Building. Expected: ':Building identifier'"_s);

                def = appendNewBuildingDef(name.value());
                building_ids.append(def->name);
            } else if (firstWord == "Intersection"_s) {
                auto name = reader.next_token();
                auto part1Name = reader.next_token();
                auto part2Name = reader.next_token();

                if (!name.has_value() || !part1Name.has_value() || !part2Name.has_value())
                    return reader.make_error_message("Couldn't parse Intersection. Expected: ':Intersection identifier part1 part2'"_s);

                def = appendNewBuildingDef(name.value());
                building_ids.append(def->name);

                def->isIntersection = true;
                def->intersectionPart1Name = catalogue->buildingNames.intern(part1Name.value());
                def->intersectionPart2Name = catalogue->buildingNames.intern(part2Name.value());
            } else if (firstWord == "Template"_s) {
                auto name = reader.next_token();
                if (!name.has_value())
                    return reader.make_error_message("Couldn't parse Template. Expected: ':Template identifier'"_s);

                def = templates.put(name.value().deprecated_to_string());
            } else {
                reader.warn("Only :Building, :Intersection or :Template definitions are supported right now."_s);
            }

            // Read ahead to count how many variants this building/intersection has.
            s32 variantCount = reader.count_occurrences_of_property_in_current_command("variant"_s);
            if (variantCount > 0) {
                def->variants = makeArray(variantCount, (BuildingVariant*)variantsMemory);
                variantsMemory += sizeof(BuildingVariant) * variantCount;
            }

        }
        // Properties!
        else {
            if (def == nullptr)
                return reader.make_error_message("Found a property before starting a :Building, :Intersection or :Template!"_s);

            if (firstWord == "build"_s) {
                auto buildMethodString = reader.next_token();
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
                    return reader.make_error_message("Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all."_s);
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
                auto token_count = reader.count_remaining_tokens_in_current_line();
                for (auto tokenIndex = 0u; tokenIndex < token_count; tokenIndex++) {
                    auto transportName = reader.next_token().release_value();

                    if (transportName == "road"_s) {
                        def->transportTypes.add(TransportType::Road);
                    } else if (transportName == "rail"_s) {
                        def->transportTypes.add(TransportType::Rail);
                    } else {
                        reader.warn("Unrecognised transport type \"{0}\"."_s, { transportName });
                    }
                }
            } else if (firstWord == "crime_protection"_s) {
                if (auto crime_protection = EffectRadius::read(reader); !crime_protection.is_error()) {
                    def->policeEffect = crime_protection.release_value();
                } else {
                    return crime_protection.release_error();
                }
            } else if (firstWord == "demolish_cost"_s) {
                if (auto demolish_cost = reader.read_int<s32>(); demolish_cost.has_value()) {
                    def->demolishCost = demolish_cost.release_value();
                } else {
                    return "Failed to read demolish_cost"_s;
                }
            } else if (firstWord == "extends"_s) {
                auto templateName = reader.next_token();
                if (!templateName.has_value()) {
                    return reader.make_error_message("Missing template name in `extends`"_s);
                }

                auto templateDef = templates.find(templateName.value().deprecated_to_string());
                if (!templateDef.has_value()) {
                    return reader.make_error_message("Could not find template named '{0}'. Templates must be defined before the buildings that use them, and in the same file."_s, { templateName.value() });
                }
                BuildingDef* tDef = templateDef.value();

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
            } else if (firstWord == "fire_protection"_s) {
                if (auto fire_protection = EffectRadius::read(reader); !fire_protection.is_error()) {
                    def->fireProtection = fire_protection.release_value();
                } else {
                    return fire_protection.release_error();
                }
            } else if (firstWord == "fire_risk"_s) {
                if (auto fire_risk = reader.read_float(); fire_risk.has_value()) {
                    def->fireRisk = fire_risk.release_value();
                } else {
                    return "Failed to read fire_risk"_s;
                }
            } else if (firstWord == "grows_in"_s) {
                auto zoneName = reader.next_token();
                if (zoneName == "r"_s) {
                    def->growsInZone = ZoneType::Residential;
                } else if (zoneName == "c"_s) {
                    def->growsInZone = ZoneType::Commercial;
                } else if (zoneName == "i"_s) {
                    def->growsInZone = ZoneType::Industrial;
                } else {
                    return reader.make_error_message("Couldn't parse grows_in. Expected use:\"grows_in r/c/i\""_s);
                }
            } else if (firstWord == "health_effect"_s) {
                if (auto health_effect = EffectRadius::read(reader); !health_effect.is_error()) {
                    def->healthEffect = health_effect.release_value();
                } else {
                    return health_effect.release_error();
                }
            } else if (firstWord == "jail_size"_s) {
                if (auto jail_size = reader.read_int<s32>(); jail_size.has_value()) {
                    def->jailCapacity = jail_size.release_value();
                } else {
                    return "Failed to read jail_size"_s;
                }
            } else if (firstWord == "jobs"_s) {
                if (auto jobs = reader.read_int<s32>(); jobs.has_value()) {
                    def->jobs = jobs.release_value();
                } else {
                    return "Failed to read jobs"_s;
                }
            } else if (firstWord == "land_value"_s) {
                if (auto land_value = EffectRadius::read(reader); !land_value.is_error()) {
                    def->landValueEffect = land_value.release_value();
                } else {
                    return land_value.release_error();
                }
            } else if (firstWord == "name"_s) {
                if (auto name = reader.next_token(); name.has_value()) {
                    def->textAssetName = asset_manager().assetStrings.intern(name.value());
                } else {
                    return reader.make_error_message("Missing value for `name`"_s);
                }
            } else if (firstWord == "pollution"_s) {
                if (auto pollution = EffectRadius::read(reader); !pollution.is_error()) {
                    def->pollutionEffect = pollution.release_value();
                } else {
                    return pollution.release_error();
                }
            } else if (firstWord == "power_gen"_s) {
                if (auto power_gen = reader.read_int<s32>(); power_gen.has_value()) {
                    def->power = power_gen.release_value();
                } else {
                    return "Failed to read power_gen"_s;
                }
            } else if (firstWord == "power_use"_s) {
                if (auto power_use = reader.read_int<s32>(); power_use.has_value()) {
                    def->power = -power_use.release_value();
                } else {
                    return "Failed to read power_use"_s;
                }
            } else if (firstWord == "requires_transport_connection"_s) {
                if (auto requires_transport_connection = reader.read_bool(); requires_transport_connection.has_value()) {
                    if (requires_transport_connection.value()) {
                        def->flags.add(BuildingFlags::RequiresTransportConnection);
                    } else {
                        def->flags.remove(BuildingFlags::RequiresTransportConnection);
                    }
                } else {
                    return "Failed to read requires_transport_connection"_s;
                }
            } else if (firstWord == "residents"_s) {
                if (auto residents = reader.read_int<s32>(); residents.has_value()) {
                    def->residents = residents.release_value();
                } else {
                    return "Failed to read residents"_s;
                }
            } else if (firstWord == "size"_s) {
                auto w = reader.read_int<s32>();
                auto h = reader.read_int<s32>();

                if (w.has_value() && h.has_value()) {
                    def->size.x = w.release_value();
                    def->size.y = h.release_value();

                    if ((def->variants.count > 0) && (def->size.x != 1 || def->size.y != 1)) {
                        return reader.make_error_message("This building is {0}x{1} and has variants. Variants are only allowed for 1x1 tile buildings!"_s, { formatInt(def->size.x), formatInt(def->size.y) });
                    }
                } else {
                    return reader.make_error_message("Couldn't parse size. Expected 2 ints (w,h)."_s);
                }
            } else if (firstWord == "sprite"_s) {
                if (auto name = reader.next_token(); name.has_value()) {
                    String spriteName = asset_manager().assetStrings.intern(name.value());
                    def->spriteName = spriteName;
                } else {
                    return reader.make_error_message("Missing name in `sprite`"_s);
                }
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

                    auto maybe_direction_flags = reader.next_token();
                    auto maybe_sprite_name = reader.next_token();
                    if (!maybe_direction_flags.has_value() || !maybe_sprite_name.has_value()) {
                        return reader.make_error_message("`variant` missing direction flags or sprite name"_s);
                    }
                    auto directionFlags = maybe_direction_flags.release_value();
                    auto spriteName = asset_manager().assetStrings.intern(maybe_sprite_name.value());

                    // Check the values are valid first, because that's less verbose than checking each one individually.
                    for (auto i = 0; i < directionFlags.length(); i++) {
                        if (!connectionTypeOf(directionFlags[i]).has_value()) {
                            return reader.make_error_message("Unrecognized connection type character '{0}', valid values: '012*'"_s, { String::repeat(directionFlags[i], 1) });
                        }
                    }

                    if (directionFlags.length() == 8) {
                        variant->connections[ConnectionDirection::N] = connectionTypeOf(directionFlags[0]).value();
                        variant->connections[ConnectionDirection::NE] = connectionTypeOf(directionFlags[1]).value();
                        variant->connections[ConnectionDirection::E] = connectionTypeOf(directionFlags[2]).value();
                        variant->connections[ConnectionDirection::SE] = connectionTypeOf(directionFlags[3]).value();
                        variant->connections[ConnectionDirection::S] = connectionTypeOf(directionFlags[4]).value();
                        variant->connections[ConnectionDirection::SW] = connectionTypeOf(directionFlags[5]).value();
                        variant->connections[ConnectionDirection::W] = connectionTypeOf(directionFlags[6]).value();
                        variant->connections[ConnectionDirection::NW] = connectionTypeOf(directionFlags[7]).value();
                    } else if (directionFlags.length() == 4) {
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
                        return reader.make_error_message("First argument for a building 'variant' should be a 4 or 8 character string consisting of 0/1/2/* flags (meaning nothing/part1/part2/anything) for N/E/S/W or N/NE/E/SE/S/SW/W/NW connectivity. eg, 101012**"_s);
                    }

                    variant->spriteName = spriteName;
                } else {
                    return reader.make_error_message("Too many variants for building '{0}'!"_s, { def->name });
                }
            } else {
                return reader.make_error_message("Unrecognized token: {0}"_s, { firstWord });
            }
        }
    }

    if (def != nullptr) {
        // Categorise the last building
        assign_building_categories(*catalogue, *def);
    }

    logInfo("Loaded {0} buildings: R:{1} C:{2} I:{3} growable, player-constructible:{4}, intersections:{5}"_s, { formatInt(catalogue->allBuildings.count), formatInt(catalogue->rGrowableBuildings.count), formatInt(catalogue->cGrowableBuildings.count), formatInt(catalogue->iGrowableBuildings.count), formatInt(catalogue->constructibleBuildings.count), formatInt(catalogue->intersectionBuildings.count) });

    return adopt_own(*new BuildingDefs(move(data), move(building_ids)));
}

BuildingDefs::BuildingDefs(Blob data, Array<String> building_ids)
    : m_data(move(data))
    , m_building_ids(move(building_ids))
{
}

void BuildingDefs::unload(AssetMetadata& metadata)
{
    auto& building_catalogue = BuildingCatalogue::the();

    for (auto const& building_id : m_building_ids) {
        BuildingDef* def = findBuildingDef(building_id);
        if (def != nullptr) {
            building_catalogue.constructibleBuildings.findAndRemove(def);
            building_catalogue.rGrowableBuildings.findAndRemove(def);
            building_catalogue.cGrowableBuildings.findAndRemove(def);
            building_catalogue.iGrowableBuildings.findAndRemove(def);
            building_catalogue.intersectionBuildings.findAndRemove(def);

            building_catalogue.buildingsByName.removeKey(building_id);

            building_catalogue.allBuildings.removeIndex(def->typeID);

            building_catalogue.buildingNameToTypeID.removeKey(building_id);
        }
    }

    Assets::assets_deallocate(m_data);
    m_building_ids = {};
}
