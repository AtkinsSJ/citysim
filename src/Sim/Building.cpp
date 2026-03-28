/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Building.h"
#include <Assets/AssetManager.h>
#include <Gfx/Colour.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>

BuildingDef const& Building::get_def() const
{
    return *getBuildingDef(typeID);
}

bool BuildingDef::has_type(BuildingType type) const
{
    if (typeID == type)
        return true;

    if (isIntersection)
        return intersectionPart1TypeID == type || intersectionPart2TypeID == type;

    return false;
}

bool BuildingDef::matches_variant(BuildingVariant const& variant, EnumMap<ConnectionDirection, Optional<BuildingDef const&>> const& neighbour_defs) const
{
    DEBUG_FUNCTION();

    bool result = true;

    for (auto direction : enum_values<ConnectionDirection>()) {
        bool matchedDirection = false;
        ConnectionType connectionType = variant.connections[direction];
        auto& neighbourDef = neighbour_defs[direction];

        if (!neighbourDef.has_value()) {
            matchedDirection = (connectionType == ConnectionType::Nothing) || (connectionType == ConnectionType::Anything);
        } else {
            switch (connectionType) {
            case ConnectionType::Nothing: {
                if (isIntersection) {
                    matchedDirection = !neighbourDef->has_type(intersectionPart1TypeID)
                        && !neighbourDef->has_type(intersectionPart2TypeID);
                } else {
                    matchedDirection = !neighbourDef->has_type(typeID);
                }
            } break;

            case ConnectionType::Building1: {
                if (isIntersection) {
                    matchedDirection = neighbourDef->has_type(intersectionPart1TypeID);
                } else {
                    matchedDirection = neighbourDef->has_type(typeID);
                }
            } break;

            case ConnectionType::Building2: {
                if (isIntersection) {
                    matchedDirection = neighbourDef->has_type(intersectionPart2TypeID);
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

        if (!matchedDirection) {
            result = false;
            break;
        }
    }

    return result;
}

void Building::update_variant(City& city, Optional<BuildingDef const&> passed_def)
{
    DEBUG_FUNCTION();

    auto const& def = passed_def.value_or(get_def());

    if (def.variants.count > 0) {
        // NB: Right now we only allow variants for 1x1 buildings.
        // Later, we might want to expand that, but it will make things a LOT more complicated, so I'm
        // starting simple!
        // The only non-1x1 cases I can think of could be worked around, too, so it's fine.
        // (eg, A train station with included rail - only the RAILS need variants, the station doesn't!)
        // - Sam, 14/02/2020

        // Calculate our connections
        s32 x = footprint.x();
        s32 y = footprint.y();

        static_assert(to_underlying(ConnectionDirection::COUNT) == 8, "updateBuildingVariant() assumes ConnectionDirectionCount == 8");
        EnumMap<ConnectionDirection, Optional<BuildingDef const&>> neighbourDefs;
        for (auto direction : enum_values<ConnectionDirection>()) {
            if (auto* building_in_direction = city.get_building_at(x + connection_offsets[direction].x, y + connection_offsets[direction].y)) {
                neighbourDefs[direction] = building_in_direction->get_def();
            }
        }

        // Search for a matching variant
        // Right now... YAY LINEAR SEARCH! @Speed
        bool foundVariant = false;
        for (s32 variant_index = 0; variant_index < def.variants.count; variant_index++) {
            auto& variant = def.variants[variant_index];
            if (def.matches_variant(variant, neighbourDefs)) {
                this->variantIndex = variant_index;
                foundVariant = true;
                logInfo("Matched building {0}#{1} with variant #{2}"_s, { def.name, formatInt(id), formatInt(variant_index) });
                break;
            }
        }

        if (!foundVariant) {
            if (!def.spriteName.is_empty()) {
                logWarn("Unable to find a matching variant for building '{0}'. Defaulting to the building's defined sprite, '{1}'."_s, { def.name, def.spriteName });
                variantIndex = {};
            } else {
                logWarn("Unable to find a matching variant for building '{0}'. Defaulting to variant #0."_s, { def.name });
                variantIndex = 0;
            }
        }
    } else {
        variantIndex = {};
    }

    // Update the entity sprite
    load_sprite();
}

Building::Building(s32 id, BuildingDef const& def, Rect2I footprint, GameTimestamp creation_date)
    : id(id)
    , typeID(def.typeID)
    , creationDate(creation_date)
    , footprint(footprint)
{
}

void Building::update(City& city)
{
    auto& def = get_def();

    // Check the building's needs are met
    // ... except for the ones that are checked by layers.

    // Distance to road
    // TODO: Replace with access to any transport types, instead of just road? Not sure what we want with that.
    if ((def.flags.has(BuildingFlags::RequiresTransportConnection)) || (def.growsInZone != ZoneType::None)) {
        s32 distanceToRoad = s32Max;
        // TODO: @Speed: We only actually need to check the boundary tiles, because they're guaranteed to be less than
        // the inner tiles... unless we allow multiple buildings per tile. Actually maybe we do? I'm not sure how that
        // would work really. Anyway, can think about that later.
        // - Sam, 30/08/2019
        for (s32 y = footprint.y(); y < footprint.y() + footprint.height(); y++) {
            for (s32 x = footprint.x(); x < footprint.x() + footprint.width(); x++) {
                distanceToRoad = min(distanceToRoad, city.transportLayer.distance_to_transport(x, y, TransportType::Road));
            }
        }

        if (def.growsInZone != ZoneType::None) {
            // Zoned buildings inherit their zone's max distance to road.
            if (distanceToRoad > ZONE_DEFS[def.growsInZone].maximumDistanceToRoad) {
                add_problem(BuildingProblem::Type::NoTransportAccess, city);
            } else {
                remove_problem(BuildingProblem::Type::NoTransportAccess);
            }
        } else if (def.flags.has(BuildingFlags::RequiresTransportConnection)) {
            // Other buildings require direct contact
            if (distanceToRoad > 1) {
                add_problem(BuildingProblem::Type::NoTransportAccess, city);
            } else {
                remove_problem(BuildingProblem::Type::NoTransportAccess);
            }
        }
    }

    // Fire!
    if (city.fireLayer.does_area_contain_fire(footprint)) {
        add_problem(BuildingProblem::Type::Fire, city);
    } else {
        remove_problem(BuildingProblem::Type::Fire);
    }

    // Power!
    if (def.power < 0) {
        if (-def.power > allocatedPower) {
            add_problem(BuildingProblem::Type::NoPower, city);
        } else {
            remove_problem(BuildingProblem::Type::NoPower);
        }
    }

    // Now, colour the building based on its problems
    auto drawColorNormal = Colour::white();
    auto drawColorNoPower = Colour::from_rgb_255(32, 32, 64, 255);

    if (!has_power()) {
        entity->color = drawColorNoPower;
    } else {
        entity->color = drawColorNormal;
    }
}

void Building::add_problem(BuildingProblem::Type problem_type, City& city)
{
    auto& problem = problems[problem_type];
    if (!problem.isActive) {
        problem.isActive = true;
        problem.type = problem_type;
        problem.startDate = city.gameClock.current_day();
    }

    // TODO: Update zots!
}

void Building::remove_problem(BuildingProblem::Type problem_type)
{
    if (problems[problem_type].isActive) {
        problems[problem_type].isActive = false;

        // TODO: Update zots!
    }
}

bool Building::has_problem(BuildingProblem::Type problem_type) const
{
    return problems[problem_type].isActive;
}

void Building::load_sprite()
{
    auto& def = get_def();
    if (variantIndex.has_value()) {
        entity->sprite = SpriteRef { def.variants[variantIndex.value()].spriteName, spriteOffset };
    } else {
        entity->sprite = SpriteRef { def.spriteName, spriteOffset };
    }
}

Optional<ConnectionType> connection_type_of(char c)
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

char as_char(ConnectionType connection_type)
{
    char result;

    switch (connection_type) {
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

s32 Building::required_power() const
{
    if (auto power = get_def().power; power < 0)
        return -power;

    return 0;
}

bool Building::has_power() const
{
    return !has_problem(BuildingProblem::Type::NoPower);
}

BuildingRef Building::get_reference() const
{
    return BuildingRef { id, footprint.position() };
}
