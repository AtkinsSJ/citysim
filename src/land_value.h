/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "dirty.h"
#include "sector.h"

struct City;
struct LandValueLayer {
    DirtyRects dirtyRects;

    SectorGrid<BasicSector> sectors;

    Array2<s16> tileBuildingContributions;

    Array2<u8> tileLandValue; // Cached total
};

s32 const maxLandValueEffectDistance = 16; // TODO: Better value for this!

void initLandValueLayer(LandValueLayer* layer, City* city, MemoryArena* gameArena);
void updateLandValueLayer(City* city, LandValueLayer* layer);
void markLandValueLayerDirty(LandValueLayer* layer, Rect2I bounds);

float getLandValuePercentAt(City* city, s32 x, s32 y);

void saveLandValueLayer(LandValueLayer* layer, struct BinaryFileWriter* writer);
bool loadLandValueLayer(LandValueLayer* layer, City* city, struct BinaryFileReader* reader);
