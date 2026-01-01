/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Assets/Forward.h>
#include <Sim/Forward.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct BuildingCatalogue final : public AssetManagerListener {
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

inline BuildingCatalogue buildingCatalogue = {};

void initBuildingCatalogue();

BuildingDef* appendNewBuildingDef(StringView name);
void loadBuildingDefs(Blob data, Asset* asset);
void removeBuildingDefs(Array<String> idsToRemove);
BuildingDef* getBuildingDef(s32 buildingTypeID);
BuildingDef* findBuildingDef(String name);

void saveBuildingTypes();
void remapBuildingTypes();
