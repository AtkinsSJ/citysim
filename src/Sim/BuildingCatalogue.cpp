/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BuildingCatalogue.h"
#include <App/App.h>
#include <Sim/Building.h>
#include <Sim/Game.h>
#include <Util/Random.h>

static BuildingCatalogue* s_building_catalogue;

BuildingCatalogue& BuildingCatalogue::the()
{
    return *s_building_catalogue;
}

void initBuildingCatalogue(MemoryArena& arena)
{
    ASSERT(!s_building_catalogue);

    BuildingCatalogue* catalogue = arena.allocate<BuildingCatalogue>();

    catalogue->constructibleBuildings = { arena, 64 };
    catalogue->rGrowableBuildings = { arena, 64 };
    catalogue->cGrowableBuildings = { arena, 64 };
    catalogue->iGrowableBuildings = { arena, 64 };
    catalogue->intersectionBuildings = { arena, 64 };

    catalogue->allBuildings = { arena, 64 };
    // NB: BuildingDef ids are 1-indexed. At least one place (BuildingDef.canBeBuiltOnID) uses 0 as a "none" value.
    // So, we have to append a blank for a "null" def. Could probably get rid of it, but initialise-to-zero is convenient
    // and I'm likely to accidentally leave other things set to 0, so it's safer to just keep the null def.
    // Update 18/02/2020: We now use the null building def when failing to match an intersection part name.
    (void)catalogue->allBuildings.append(); // Null building def

    catalogue->buildingNameToTypeID.put({}, 0);

    catalogue->maxRBuildingDim = 0;
    catalogue->maxCBuildingDim = 0;
    catalogue->maxIBuildingDim = 0;
    catalogue->overallMaxBuildingDim = 0;

    asset_manager().register_listener(catalogue);
    s_building_catalogue = catalogue;
}

s32 BuildingCatalogue::get_max_building_size(ZoneType zone_type) const
{
    switch (zone_type) {
    case ZoneType::Residential:
        return maxRBuildingDim;
    case ZoneType::Commercial:
        return maxCBuildingDim;
    case ZoneType::Industrial:
        return maxIBuildingDim;
    case ZoneType::None:
        return 0;
    case ZoneType::COUNT:
        VERIFY_NOT_REACHED();
    }
    VERIFY_NOT_REACHED();
}

Optional<BuildingDef const&> BuildingCatalogue::find_building_intersection(BuildingDef const& a, BuildingDef const& b) const
{
    DEBUG_FUNCTION();

    // It's horrible linear search time!
    for (auto it = intersectionBuildings.iterate(); it.hasNext(); it.next()) {
        BuildingDef* itDef = it.getValue();

        if (itDef->isIntersection) {
            if ((itDef->intersectionPart1TypeID == a.typeID && itDef->intersectionPart2TypeID == b.typeID)
                || (itDef->intersectionPart2TypeID == a.typeID && itDef->intersectionPart1TypeID == b.typeID)) {
                return *itDef;
            }
        }
    }

    return {};
}

Optional<BuildingDef const&> BuildingCatalogue::find_random_zone_building(ZoneType zone_type, Random& random, Function<bool(BuildingDef const&)> filter) const
{
    DEBUG_FUNCTION();

    // Choose a random building, then carry on checking buildings until one is acceptable
    auto const& buildings = [this, zone_type] {
        switch (zone_type) {
        case ZoneType::Residential:
            return rGrowableBuildings;
        case ZoneType::Commercial:
            return cGrowableBuildings;
        case ZoneType::Industrial:
            return iGrowableBuildings;

            INVALID_DEFAULT_CASE;
        }
        VERIFY_NOT_REACHED();
    }();

    // TODO: @RandomIterate - This random selection is biased, and wants replacing with an iteration only over valid options,
    // like in "growSomeZoneBuildings - find a valid zone".
    // Well, it does if growing buildings one at a time is how we want to do things. I'm not sure.
    // Growing a whole "block" of a building might make more sense for residential at least.
    // Something to decide on later.
    // - Sam, 18/08/2019
    for (auto it = buildings.iterate(random.random_below(truncate32(buildings.count)));
        it.hasNext();
        it.next()) {
        BuildingDef& def = *it.getValue();

        if (filter(def))
            return def;
    }

    return {};
}

void BuildingCatalogue::after_assets_loaded()
{
    if (auto* game_scene = dynamic_cast<GameScene*>(&App::the().scene())) {
        if (auto* city = game_scene->city())
            remap_building_types(*city);
    }
}

BuildingDef* appendNewBuildingDef(StringView name)
{
    auto& building_catalogue = BuildingCatalogue::the();
    Indexed<BuildingDef> newDef = building_catalogue.allBuildings.append();
    BuildingDef& result = newDef.value();
    result.name = building_catalogue.buildingNames.intern(name);
    result.typeID = newDef.index();

    result.fireRisk = 1.0f;
    building_catalogue.buildingsByName.put(result.name, &result);
    building_catalogue.buildingNameToTypeID.put(result.name, result.typeID);

    return &result;
}

BuildingDef* getBuildingDef(BuildingType buildingTypeID)
{
    auto& building_catalogue = BuildingCatalogue::the();
    BuildingDef* result = building_catalogue.allBuildings.get(0);

    if (buildingTypeID > 0 && buildingTypeID < building_catalogue.allBuildings.count) {
        BuildingDef* found = building_catalogue.allBuildings.get(buildingTypeID);
        if (found != nullptr)
            result = found;
    }

    return result;
}

BuildingDef* findBuildingDef(String name)
{
    BuildingDef* result = BuildingCatalogue::the().buildingsByName.find_value(name).value_or(nullptr);

    return result;
}

void saveBuildingTypes()
{
    auto& building_catalogue = BuildingCatalogue::the();
    // Post-processing of BuildingDefs
    for (auto it = building_catalogue.allBuildings.iterate(); it.hasNext(); it.next()) {
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
    building_catalogue.buildingNameToOldTypeID.put_all(building_catalogue.buildingNameToTypeID);
}

void BuildingCatalogue::remap_building_types(City& city)
{
    // FIXME: This doesn't seem to work any more. Investigate!

    // First, remap any IDs that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = buildingNameToOldTypeID.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        buildingNameToTypeID.ensure(entry->key, buildingNameToTypeID.count());
    }

    if (buildingNameToOldTypeID.count() > 0) {
        Array<BuildingType> oldTypeToNewType = temp_arena().allocate_filled_array<BuildingType>(buildingNameToOldTypeID.count());
        for (auto it = buildingNameToOldTypeID.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String buildingName = entry->key;
            auto oldType = entry->value;

            oldTypeToNewType[oldType] = buildingNameToTypeID.find_value(buildingName).value_or(0);
        }

        for (auto it = city.buildings.iterate(); it.hasNext(); it.next()) {
            Building* building = it.get();
            auto oldType = building->typeID;
            if (oldType < oldTypeToNewType.count() && (oldTypeToNewType[oldType] != 0)) {
                building->typeID = oldTypeToNewType[oldType];
            }
        }
    }

    saveBuildingTypes();
}
