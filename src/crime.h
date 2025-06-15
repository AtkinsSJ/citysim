/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "building.h"
#include "dirty.h"
#include "sector.h"

struct CrimeLayer {
    DirtyRects dirtyRects;

    SectorGrid<BasicSector> sectors;

    Array2<u8> tilePoliceCoverage;

    ChunkedArray<BuildingRef> policeBuildings;
    s32 totalJailCapacity;
    s32 occupiedJailCapacity;

    f32 fundingLevel; // @Budget
};

void initCrimeLayer(CrimeLayer* layer, City* city, MemoryArena* gameArena);
void updateCrimeLayer(City* city, CrimeLayer* layer);

void notifyNewBuilding(CrimeLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(CrimeLayer* layer, BuildingDef* def, Building* building);

f32 getPoliceCoveragePercentAt(City* city, s32 x, s32 y);

void saveCrimeLayer(CrimeLayer* layer, struct BinaryFileWriter* writer);
bool loadCrimeLayer(CrimeLayer* layer, City* city, struct BinaryFileReader* reader);
