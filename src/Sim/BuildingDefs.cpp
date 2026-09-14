/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BuildingDefs.h"

#include <Assets/AssetManager.h>
#include <Sim/Building.h>
#include <Sim/BuildingCatalogue.h>
#include <Util/HashMap.h>
#include <Util/Lexer.h>
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

template<typename T>
static ErrorOr<EnumMap<T, u32>> read_tagged_u32_series_property(StringView property_name, Lexer& lexer)
{
    EnumMap<T, u32> result;
    bool any_values_defined = false;

    while (lexer.has_next()) {
        auto type_name = lexer.consume_until(':');
        bool had_colon = lexer.consume_specific(':');
        auto count = lexer.consume_int<u32>();
        lexer.discard_whitespace();

        if (!type_name.has_value() || !had_colon || !count.has_value())
            return "Invalid definition, expected a series of `NAME:COUNT` separated by spaces."_s;

        auto type = enum_from_string<T>(type_name.value());
        if (!type.has_value())
            return myprintf("Unrecognized type `{}`"_s, { type_name.value() });

        any_values_defined = true;
        result[type.value()] = count.value();
    }

    if (!any_values_defined)
        return myprintf("Couldn't parse {}: No values defined."_s, { property_name });
    return result;
}

ErrorOr<OwnedRef<BuildingDefs>> BuildingDefs::load(AssetMetadata& metadata, Blob file_data)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, file_data };

    BuildingCatalogue* catalogue = &BuildingCatalogue::the();

    // Count the number of building defs in the file first, so we can allocate the building_ids array in the asset
    size_t buildingCount = 0;
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
    auto data = asset_manager().allocate_blob(buildingNamesSize + variantsSize);
    Array<String> building_ids { buildingCount, reinterpret_cast<String*>(data.writable_data()) };
    u8* variantsMemory = data.writable_data() + buildingNamesSize;

    reader.restart();

    HashMap<String, BuildingDef> templates;

    BuildingDef* def = nullptr;

    while (reader.load_next_line()) {
        Lexer lexer { reader.current_line() };

        // Commands
        if (lexer.consume_specific(':')) {
            // Define something
            auto command = lexer.consume_token();
            lexer.discard_whitespace();

            if (def != nullptr) {
                // Now that the previous building is done, we can categorise it
                assign_building_categories(*catalogue, *def);
            }

            if (command == "Building"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                if (!name.has_value() || lexer.has_next())
                    return reader.make_error_message("Couldn't parse Building. Expected: ':Building identifier'"_s);

                def = appendNewBuildingDef(name.value());
                building_ids.append(def->name);
            } else if (command == "Intersection"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                auto part1Name = lexer.consume_token();
                lexer.discard_whitespace();
                auto part2Name = lexer.consume_token();
                lexer.discard_whitespace();

                if (!name.has_value() || !part1Name.has_value() || !part2Name.has_value() || lexer.has_next())
                    return reader.make_error_message("Couldn't parse Intersection. Expected: ':Intersection identifier part1 part2'"_s);

                def = appendNewBuildingDef(name.value());
                building_ids.append(def->name);

                def->isIntersection = true;
                def->intersectionPart1Name = catalogue->buildingNames.intern(part1Name.value());
                def->intersectionPart2Name = catalogue->buildingNames.intern(part2Name.value());
            } else if (command == "Template"_s) {
                auto name = lexer.consume_token();
                lexer.discard_whitespace();
                if (!name.has_value() || lexer.has_next())
                    return reader.make_error_message("Couldn't parse Template. Expected: ':Template identifier'"_s);

                def = &templates.set(name.value().deprecated_to_string(), {});
            } else {
                reader.warn("Only :Building, :Intersection or :Template definitions are supported right now."_s);
            }

            // Read ahead to count how many variants this building/intersection has.
            auto variant_count = reader.count_occurrences_of_property_in_current_command("variant"_s);
            if (variant_count > 0) {
                def->variants = { variant_count, reinterpret_cast<BuildingVariant*>(variantsMemory) };
                variantsMemory += sizeof(BuildingVariant) * variant_count;
            }
            continue;
        }

        auto maybe_property = lexer.consume_token();
        if (!maybe_property.has_value())
            continue;
        auto property_name = maybe_property.release_value();
        lexer.discard_whitespace();

        // Properties!
        if (def == nullptr)
            return reader.make_error_message("Found a property before starting a :Building, :Intersection or :Template!"_s);

        auto read_optional_bool_property = [&]() -> ErrorOr<bool> {
            auto const bool_value = lexer.consume_bool();
            lexer.discard_whitespace();
            if (lexer.has_next())
                return reader.make_error_message("Couldn't parse {0}. Expected: `{0} [BOOLEAN]` (default true)"_s, { property_name });
            return bool_value != false;
        };

        auto read_effect_radius_property = [&]() -> ErrorOr<EffectRadius> {
            auto effect = EffectRadius::read(lexer);
            lexer.discard_whitespace();
            if (!effect.has_value() || lexer.has_next()) {
                return reader.make_error_message("Couldn't parse {0}. Expected: `{0} RADIUS [EFFECT_AT_CENTRE] [EFFECT_AT_EDGE]` where all 3 parameters are ints."_s, { property_name });
            }
            return effect.release_value();
        };

        auto read_s32_property = [&]() -> ErrorOr<s32> {
            auto int_value = lexer.consume_int<s32>();
            lexer.discard_whitespace();
            if (!int_value.has_value() || lexer.has_next()) {
                return reader.make_error_message("Couldn't parse {0}. Expected: `{0} INTEGER`"_s, { property_name });
            }
            return int_value.release_value();
        };

        if (property_name == "build"_s) {
            auto build_method_name = lexer.consume_token();
            lexer.discard_whitespace();
            auto cost = lexer.consume_int<s32>();
            lexer.discard_whitespace();

            if (!build_method_name.has_value() || !cost.has_value() || lexer.has_next()) {
                return reader.make_error_message("Couldn't parse build. Expected use:\"build method cost\", where method is (plop/line/rect). If it's not buildable, just don't have a \"build\" line at all."_s);
            }

            if (build_method_name == "paint"_s) {
                def->buildMethod = BuildMethod::Paint;
            } else if (build_method_name == "plop"_s) {
                def->buildMethod = BuildMethod::Plop;
            } else if (build_method_name == "line"_s) {
                def->buildMethod = BuildMethod::DragLine;
            } else if (build_method_name == "rect"_s) {
                def->buildMethod = BuildMethod::DragRect;
            } else {
                return reader.make_error_message("Couldn't parse the build method."_s);
            }
            def->buildCost = cost.release_value();

        } else if (property_name == "carries_power"_s) {
            auto carries_power = read_optional_bool_property();
            if (carries_power.is_error())
                return carries_power.release_error();
            def->flags.set(BuildingFlags::CarriesPower, carries_power.value());
        } else if (property_name == "carries_transport"_s) {
            if (!lexer.has_next()) {
                return reader.make_error_message("Couldn't parse carries_transport. Expected: `carries_transport` followed by a list of transport names (`road`, `rail`)"_s);
            }
            while (lexer.has_next()) {
                auto transport_name = lexer.consume_token();
                lexer.discard_whitespace();

                if (!transport_name.has_value()) {
                    return reader.make_error_message("Couldn't parse carries_transport. Expected: `carries_transport` followed by a list of transport names (`road`, `rail`)"_s);
                }

                if (transport_name == "road"_s) {
                    def->transportTypes.add(TransportType::Road);
                } else if (transport_name == "rail"_s) {
                    def->transportTypes.add(TransportType::Rail);
                } else {
                    return reader.make_error_message("Unrecognised transport type \"{0}\"."_s, { transport_name.value() });
                }
            }
        } else if (property_name == "crime_protection"_s) {
            auto effect = read_effect_radius_property();
            if (effect.is_error())
                return effect.release_error();
            def->policeEffect = effect.release_value();
        } else if (property_name == "demolish_cost"_s) {
            auto demolish_cost = read_s32_property();
            if (demolish_cost.is_error())
                return demolish_cost.release_error();
            def->demolishCost = demolish_cost.release_value();
        } else if (property_name == "extends"_s) {
            auto template_name = lexer.consume_token();
            lexer.discard_whitespace();
            if (!template_name.has_value() || lexer.has_next()) {
                return reader.make_error_message("Couldn't parse extends. Expected: `extends NAME`"_s);
            }

            auto maybe_template_def = templates.get(template_name.value().deprecated_to_string());
            if (!maybe_template_def.has_value()) {
                return reader.make_error_message("Could not find template named '{0}'. Templates must be defined before the buildings that use them, and in the same file."_s, { template_name.value() });
            }
            auto const& template_def = maybe_template_def.value();

            // Copy the def... this could be messy
            // (We can't just do copyMemory() because we don't want to change the name or typeID.)
            def->flags = template_def.flags;
            def->size = template_def.size;
            def->spriteName = template_def.spriteName;
            def->buildMethod = template_def.buildMethod;
            def->buildCost = template_def.buildCost;
            def->growsInZone = template_def.growsInZone;
            def->demolishCost = template_def.demolishCost;
            def->residents = template_def.residents;
            def->jobs = template_def.jobs;
            def->transportTypes = template_def.transportTypes;
            def->power = template_def.power;
            def->landValueEffect = template_def.landValueEffect;
            def->pollutionEffect = template_def.pollutionEffect;
            def->fireRisk = template_def.fireRisk;
            def->fireProtection = template_def.fireProtection;
        } else if (property_name == "fire_protection"_s) {
            auto effect = read_effect_radius_property();
            if (effect.is_error())
                return effect.release_error();
            def->fireProtection = effect.release_value();
        } else if (property_name == "fire_risk"_s) {
            auto fire_risk = lexer.consume_float<float>();
            lexer.discard_whitespace();
            if (fire_risk.has_value() && !lexer.has_next()) {
                def->fireRisk = fire_risk.release_value();
            } else {
                return reader.make_error_message("Failed to read fire_risk"_s);
            }
        } else if (property_name == "grows_in"_s) {
            // FIXME: We probably want zone names to be dynamic here, and allow multiple zones per building.
            auto zone_name = lexer.consume_token();
            if (zone_name == "r"_s) {
                def->growsInZone = ZoneType::Residential;
            } else if (zone_name == "c"_s) {
                def->growsInZone = ZoneType::Commercial;
            } else if (zone_name == "i"_s) {
                def->growsInZone = ZoneType::Industrial;
            } else {
                return reader.make_error_message("Couldn't parse grows_in. Expected: `grows_in ZONE` where ZONE is r/c/i"_s);
            }
        } else if (property_name == "health_effect"_s) {
            auto effect = read_effect_radius_property();
            if (effect.is_error())
                return effect.release_error();
            def->healthEffect = effect.release_value();
        } else if (property_name == "jail_size"_s) {
            auto jail_size = read_s32_property();
            if (jail_size.is_error())
                return jail_size.release_error();
            def->jailCapacity = jail_size.release_value();
        } else if (property_name == "jobs"_s) {
            // This replaces any `jobs` defined by a template. I think that's desirable.
            auto jobs = read_tagged_u32_series_property<JobType>(property_name, lexer);
            if (jobs.is_error())
                return reader.make_error_message(jobs.release_error());
            def->jobs = jobs.release_value();
        } else if (property_name == "land_value"_s) {
            auto effect = read_effect_radius_property();
            if (effect.is_error())
                return effect.release_error();
            def->landValueEffect = effect.release_value();
        } else if (property_name == "name"_s) {
            auto name = lexer.consume_token();
            lexer.discard_whitespace();
            if (name.has_value() && !lexer.has_next()) {
                def->textAssetName = asset_manager().assetStrings.intern(name.value());
            } else {
                return reader.make_error_message("Failed to parse name. Expected: `name NAME`"_s);
            }
        } else if (property_name == "pollution"_s) {
            auto effect = read_effect_radius_property();
            if (effect.is_error())
                return effect.release_error();
            def->pollutionEffect = effect.release_value();
        } else if (property_name == "power_gen"_s) {
            auto power_gen = read_s32_property();
            if (power_gen.is_error())
                return power_gen.release_error();
            def->power = power_gen.release_value();
        } else if (property_name == "power_use"_s) {
            auto power_use = read_s32_property();
            if (power_use.is_error())
                return power_use.release_error();
            def->power = -power_use.release_value();
        } else if (property_name == "requires_transport_connection"_s) {
            auto carries_power = read_optional_bool_property();
            if (carries_power.is_error())
                return carries_power.release_error();
            def->flags.set(BuildingFlags::RequiresTransportConnection, carries_power.value());
        } else if (property_name == "residents"_s) {
            // This replaces any `residents` defined by a template. I think that's desirable.
            auto residents = read_tagged_u32_series_property<ResidentType>(property_name, lexer);
            if (residents.is_error())
                return reader.make_error_message(residents.release_error());
            def->residents = residents.release_value();
        } else if (property_name == "size"_s) {
            auto size = V2I::read_size(lexer);
            lexer.discard_whitespace();
            if (!size.has_value() || lexer.has_next()) {
                return reader.make_error_message("Failed to parse size. Expected: `size #x#` where both #s are positive integers."_s);
            }

            def->size = size.release_value();
            if ((def->variants.count() > 0) && (def->size.x != 1 || def->size.y != 1)) {
                return reader.make_error_message("This building is {0}x{1} and has variants. Variants are only allowed for 1x1 tile buildings!"_s, { formatInt(def->size.x), formatInt(def->size.y) });
            }
        } else if (property_name == "sprite"_s) {
            auto name = lexer.consume_token();
            lexer.discard_whitespace();
            if (name.has_value() && !lexer.has_next()) {
                String spriteName = asset_manager().assetStrings.intern(name.value());
                def->spriteName = spriteName;
            } else {
                return reader.make_error_message("Missing name in `sprite`"_s);
            }
        } else if (property_name == "variant"_s) {
            //
            // NB: Not really related to this code but I needed somewhere to put this:
            // Right now, we linearly search through variants and pick the first one
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

            if (def->variants.count() < def->variants.capacity()) {
                BuildingVariant* variant = def->variants.append();

                auto maybe_direction_flags = lexer.consume_token();
                lexer.discard_whitespace();
                auto maybe_sprite_name = lexer.consume_token();
                lexer.discard_whitespace();
                if (!maybe_direction_flags.has_value() || !maybe_sprite_name.has_value() || lexer.has_next()) {
                    return reader.make_error_message("Failed to read variant. Expected `variant DIRECTION_FLAGS SPRITE_NAME`"_s);
                }
                auto direction_flags = maybe_direction_flags.release_value();
                auto sprite_name = asset_manager().assetStrings.intern(maybe_sprite_name.value());

                // Check the values are valid first, because that's less verbose than checking each one individually.
                for (auto i = 0; i < direction_flags.length(); i++) {
                    if (!connection_type_of(direction_flags[i]).has_value()) {
                        return reader.make_error_message("Unrecognized connection type character '{0}', valid values: '012*'"_s, { String::repeat(direction_flags[i], 1) });
                    }
                }

                if (direction_flags.length() == 8) {
                    variant->connections[ConnectionDirection::N] = connection_type_of(direction_flags[0]).value();
                    variant->connections[ConnectionDirection::NE] = connection_type_of(direction_flags[1]).value();
                    variant->connections[ConnectionDirection::E] = connection_type_of(direction_flags[2]).value();
                    variant->connections[ConnectionDirection::SE] = connection_type_of(direction_flags[3]).value();
                    variant->connections[ConnectionDirection::S] = connection_type_of(direction_flags[4]).value();
                    variant->connections[ConnectionDirection::SW] = connection_type_of(direction_flags[5]).value();
                    variant->connections[ConnectionDirection::W] = connection_type_of(direction_flags[6]).value();
                    variant->connections[ConnectionDirection::NW] = connection_type_of(direction_flags[7]).value();
                } else if (direction_flags.length() == 4) {
                    // The 4 other directions don't matter
                    variant->connections[ConnectionDirection::NE] = ConnectionType::Anything;
                    variant->connections[ConnectionDirection::SE] = ConnectionType::Anything;
                    variant->connections[ConnectionDirection::SW] = ConnectionType::Anything;
                    variant->connections[ConnectionDirection::NW] = ConnectionType::Anything;

                    variant->connections[ConnectionDirection::N] = connection_type_of(direction_flags[0]).value();
                    variant->connections[ConnectionDirection::E] = connection_type_of(direction_flags[1]).value();
                    variant->connections[ConnectionDirection::S] = connection_type_of(direction_flags[2]).value();
                    variant->connections[ConnectionDirection::W] = connection_type_of(direction_flags[3]).value();
                } else {
                    return reader.make_error_message("First argument for a building 'variant' should be a 4 or 8 character string consisting of 0/1/2/* flags (meaning nothing/part1/part2/anything) for N/E/S/W or N/NE/E/SE/S/SW/W/NW connectivity. eg, 101012**"_s);
                }

                variant->spriteName = sprite_name;
            } else {
                return reader.make_error_message("Too many variants for building '{0}'!"_s, { def->name });
            }
        } else {
            return reader.make_error_message("Unrecognized token: {0}"_s, { property_name });
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

            building_catalogue.buildingsByName.remove(building_id);

            building_catalogue.allBuildings.removeIndex(def->typeID);

            building_catalogue.buildingNameToTypeID.remove(building_id);
        }
    }

    asset_manager().deallocate(m_data);
    m_building_ids = {};
}
