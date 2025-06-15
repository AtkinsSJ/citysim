/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "dirty.h"

struct City;

struct PollutionLayer {
    DirtyRects dirtyRects;

    Array2<s16> tileBuildingContributions;

    Array2<u8> tilePollution; // Cached total
};

s32 const maxPollutionEffectDistance = 16; // TODO: Better value for this!

void initPollutionLayer(PollutionLayer* layer, City* city, MemoryArena* gameArena);
void updatePollutionLayer(City* city, PollutionLayer* layer);
void markPollutionLayerDirty(PollutionLayer* layer, Rect2I bounds);

u8 getPollutionAt(City* city, s32 x, s32 y);
f32 getPollutionPercentAt(City* city, s32 x, s32 y);

void savePollutionLayer(PollutionLayer* layer, struct BinaryFileWriter* writer);
bool loadPollutionLayer(PollutionLayer* layer, City* city, struct BinaryFileReader* reader);
