/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/BuildingRef.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Sector.h>

struct HealthLayer {
    DirtyRects dirtyRects;

    SectorGrid<BasicSector> sectors;

    Array2<u8> tileHealthCoverage;

    ChunkedArray<BuildingRef> healthBuildings;

    float fundingLevel; // @Budget
};

void initHealthLayer(HealthLayer* layer, City* city, MemoryArena* gameArena);
void updateHealthLayer(City* city, HealthLayer* layer);
void markHealthLayerDirty(HealthLayer* layer, Rect2I bounds);

void notifyNewBuilding(HealthLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(HealthLayer* layer, BuildingDef* def, Building* building);

float getHealthCoveragePercentAt(City* city, s32 x, s32 y);

void saveHealthLayer(HealthLayer* layer, BinaryFileWriter* writer);
bool loadHealthLayer(HealthLayer* layer, City* city, BinaryFileReader* reader);
