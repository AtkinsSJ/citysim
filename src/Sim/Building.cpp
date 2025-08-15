/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Building.h"
#include "../AppState.h"
#include <Assets/AssetManager.h>
#include <Gfx/Colour.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>

BuildingDef* getBuildingDef(Building* building)
{
    BuildingDef* result = nullptr;

    if (building != nullptr) {
        result = getBuildingDef(building->typeID);
    }

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

    BuildingDef* def = (passedDef != nullptr) ? passedDef : getBuildingDef(building);

    if (def->variants.count > 0) {
        // NB: Right now we only allow variants for 1x1 buildings.
        // Later, we might want to expand that, but it will make things a LOT more complicated, so I'm
        // starting simple!
        // The only non-1x1 cases I can think of could be worked around, too, so it's fine.
        // (eg, A train station with included rail - only the RAILS need variants, the station doesn't!)
        // - Sam, 14/02/2020

        // Calculate our connections
        s32 x = building->footprint.x();
        s32 y = building->footprint.y();

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
            if (!def->spriteName.is_empty()) {
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

    for (s32 y = footprint.y();
        y < footprint.y() + footprint.height();
        y++) {
        Building* buildingL = getBuildingAt(city, footprint.x() - 1, y);
        if (buildingL) {
            BuildingDef* defL = getBuildingDef(buildingL);
            if (defL->variants.count > 0)
                updateBuildingVariant(city, buildingL, defL);
        }

        Building* buildingR = getBuildingAt(city, footprint.x() + footprint.width(), y);
        if (buildingR) {
            BuildingDef* defD = getBuildingDef(buildingR);
            if (defD->variants.count > 0)
                updateBuildingVariant(city, buildingR, defD);
        }
    }

    for (s32 x = footprint.x();
        x < footprint.x() + footprint.width();
        x++) {
        Building* buildingU = getBuildingAt(city, x, footprint.y() - 1);
        Building* buildingD = getBuildingAt(city, x, footprint.y() + footprint.height());
        if (buildingU) {
            BuildingDef* defU = getBuildingDef(buildingU);
            if (defU->variants.count > 0)
                updateBuildingVariant(city, buildingU, defU);
        }
        if (buildingD) {
            BuildingDef* defR = getBuildingDef(buildingD);
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
    BuildingDef* def = getBuildingDef(building);

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
        for (s32 y = building->footprint.y(); y < building->footprint.y() + building->footprint.height(); y++) {
            for (s32 x = building->footprint.x(); x < building->footprint.x() + building->footprint.width(); x++) {
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
    BuildingDef* def = getBuildingDef(building);

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

    BuildingDef* def = getBuildingDef(building);
    if (def->power < 0) {
        result = -def->power;
    }

    return result;
}

bool buildingHasPower(Building* building)
{
    return !hasProblem(building, BuildingProblem::Type::NoPower);
}

BuildingRef Building::get_reference() const
{
    return BuildingRef { id, footprint.position() };
}
