/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "building.h"
#include "dirty.h"
#include "sector.h"

struct HealthLayer {
    DirtyRects dirtyRects;

    SectorGrid<BasicSector> sectors;

    Array2<u8> tileHealthCoverage;

    ChunkedArray<BuildingRef> healthBuildings;

    f32 fundingLevel; // @Budget
};

void initHealthLayer(HealthLayer* layer, City* city, MemoryArena* gameArena);
void updateHealthLayer(City* city, HealthLayer* layer);
void markHealthLayerDirty(HealthLayer* layer, Rect2I bounds);

void notifyNewBuilding(HealthLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(HealthLayer* layer, BuildingDef* def, Building* building);

f32 getHealthCoveragePercentAt(City* city, s32 x, s32 y);

void saveHealthLayer(HealthLayer* layer, struct BinaryFileWriter* writer);
bool loadHealthLayer(HealthLayer* layer, City* city, struct BinaryFileReader* reader);
