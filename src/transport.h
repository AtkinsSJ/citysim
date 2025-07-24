/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "dirty.h"
#include "sector.h"
#include <Sim/Forward.h>
#include <UI/Forward.h>
#include <Util/DeprecatedFlags.h>

enum TransportType {
    Transport_Road,
    Transport_Rail,
    TransportTypeCount
};

struct TransportLayer {
    DirtyRects dirtyRects;

    Array2<u8> tileTransportTypes;

    u8 transportMaxDistance;
    Array2<u8> tileTransportDistance[TransportTypeCount];
};

using Flags_TransportType = DeprecatedFlags<TransportType, u8>;

void initTransportLayer(TransportLayer* layer, City* city, MemoryArena* gameArena);
void updateTransportLayer(City* city, TransportLayer* layer);
void markTransportLayerDirty(TransportLayer* layer, Rect2I bounds);

void addTransportToTile(City* city, s32 x, s32 y, TransportType type);
void addTransportToTile(City* city, s32 x, s32 y, Flags_TransportType types);
bool doesTileHaveTransport(City* city, s32 x, s32 y, TransportType type);
bool doesTileHaveTransport(City* city, s32 x, s32 y, Flags_TransportType types);
s32 getDistanceToTransport(City* city, s32 x, s32 y, TransportType type);

void debugInspectTransport(UI::Panel* panel, City* city, s32 x, s32 y);

void saveTransportLayer(TransportLayer* layer, struct BinaryFileWriter* writer);
bool loadTransportLayer(TransportLayer* layer, City* city, struct BinaryFileReader* reader);
