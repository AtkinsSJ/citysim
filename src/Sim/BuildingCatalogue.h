/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Util/Function.h"

#include <Assets/AssetManagerListener.h>
#include <Assets/Forward.h>
#include <Sim/Forward.h>
#include <Sim/Zone.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct BuildingCatalogue final : public AssetManagerListener {
    static BuildingCatalogue& the();

    ChunkedArray<BuildingDef*> const& constructible_buildings() const { return constructibleBuildings; }
    s32 get_max_building_size(ZoneType) const;
    Optional<BuildingDef const&> find_building_intersection(BuildingDef const&, BuildingDef const&) const;

    Optional<BuildingDef const&> find_random_zone_building(ZoneType, Random&, Function<bool(BuildingDef const&)> filter) const;

    OccupancyArray<BuildingDef> allBuildings;
    HashTable<BuildingDef*> buildingsByName { 128 };
    StringTable buildingNames;

    HashTable<s32> buildingNameToTypeID { 128 };
    HashTable<s32> buildingNameToOldTypeID { 128 };

    ChunkedArray<BuildingDef*> constructibleBuildings;
    ChunkedArray<BuildingDef*> rGrowableBuildings;
    ChunkedArray<BuildingDef*> cGrowableBuildings;
    ChunkedArray<BuildingDef*> iGrowableBuildings;
    ChunkedArray<BuildingDef*> intersectionBuildings;

    s32 maxRBuildingDim;
    s32 maxCBuildingDim;
    s32 maxIBuildingDim;
    s32 overallMaxBuildingDim;

    // ^AssetManagerListener
    virtual void after_assets_loaded() override;
};

void initBuildingCatalogue(MemoryArena&);

BuildingDef* appendNewBuildingDef(StringView name);
BuildingDef* getBuildingDef(s32 buildingTypeID);
BuildingDef* findBuildingDef(String name);

void saveBuildingTypes();
void remapBuildingTypes();
