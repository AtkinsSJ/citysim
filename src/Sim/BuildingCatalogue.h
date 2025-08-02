/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Sim/Forward.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct BuildingCatalogue {
    OccupancyArray<BuildingDef> allBuildings;
    HashTable<BuildingDef*> buildingsByName;
    StringTable buildingNames;

    HashTable<s32> buildingNameToTypeID;
    HashTable<s32> buildingNameToOldTypeID;

    ChunkedArray<BuildingDef*> constructibleBuildings;
    ChunkedArray<BuildingDef*> rGrowableBuildings;
    ChunkedArray<BuildingDef*> cGrowableBuildings;
    ChunkedArray<BuildingDef*> iGrowableBuildings;
    ChunkedArray<BuildingDef*> intersectionBuildings;

    s32 maxRBuildingDim;
    s32 maxCBuildingDim;
    s32 maxIBuildingDim;
    s32 overallMaxBuildingDim;
};

inline BuildingCatalogue buildingCatalogue = {};

void initBuildingCatalogue();

BuildingDef* appendNewBuildingDef(String name);
void loadBuildingDefs(Blob data, Asset* asset);
void removeBuildingDefs(Array<String> idsToRemove);
BuildingDef* getBuildingDef(s32 buildingTypeID);
BuildingDef* findBuildingDef(String name);

void saveBuildingTypes();
void remapBuildingTypes(City* city);
