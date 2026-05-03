/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Sim/Forward.h>
#include <Sim/Zone.h>
#include <Util/ChunkedArray.h>
#include <Util/Function.h>
#include <Util/HashMap.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct BuildingCatalogue final : public AssetManagerListener {
    static BuildingCatalogue& the();

    ChunkedArray<BuildingDef*> const& constructible_buildings() const { return constructibleBuildings; }
    s32 get_max_building_size(ZoneType) const;
    Optional<BuildingDef const&> find_building_intersection(BuildingDef const&, BuildingDef const&) const;

    Optional<BuildingDef const&> find_random_zone_building(ZoneType, Random&, Function<bool(BuildingDef const&)> filter) const;

    OccupancyArray<BuildingDef> allBuildings;
    HashMap<String, BuildingDef*> buildingsByName { 128 };
    StringTable buildingNames;

    HashMap<String, BuildingType> buildingNameToTypeID { 128 };
    HashMap<String, BuildingType> buildingNameToOldTypeID { 128 };

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

    void remap_building_types(City& city);
};

void initBuildingCatalogue(MemoryArena&);

BuildingDef* appendNewBuildingDef(StringView name);
BuildingDef* getBuildingDef(BuildingType buildingTypeID);
BuildingDef* findBuildingDef(String name);

void saveBuildingTypes();
