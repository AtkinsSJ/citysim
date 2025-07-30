/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>

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
float getPollutionPercentAt(City* city, s32 x, s32 y);

void savePollutionLayer(PollutionLayer* layer, BinaryFileWriter* writer);
bool loadPollutionLayer(PollutionLayer* layer, City* city, BinaryFileReader* reader);
