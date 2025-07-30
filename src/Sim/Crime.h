/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Sector.h>
#include <Util/Forward.h>

struct CrimeLayer {
    DirtyRects dirtyRects;

    SectorGrid<BasicSector> sectors;

    Array2<u8> tilePoliceCoverage;

    ChunkedArray<BuildingRef> policeBuildings;
    s32 totalJailCapacity;
    s32 occupiedJailCapacity;

    float fundingLevel; // @Budget
};

void initCrimeLayer(CrimeLayer* layer, City* city, MemoryArena* gameArena);
void updateCrimeLayer(City* city, CrimeLayer* layer);

void notifyNewBuilding(CrimeLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(CrimeLayer* layer, BuildingDef* def, Building* building);

float getPoliceCoveragePercentAt(City* city, s32 x, s32 y);

void saveCrimeLayer(CrimeLayer* layer, BinaryFileWriter* writer);
bool loadCrimeLayer(CrimeLayer* layer, City* city, BinaryFileReader* reader);
