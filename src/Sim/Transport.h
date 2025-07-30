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
#include <UI/Forward.h>
#include <Util/Flags.h>

enum class TransportType : u8 {
    Road,
    Rail,
    COUNT
};

struct TransportLayer {
    DirtyRects dirtyRects;

    Array2<Flags<TransportType>> tileTransportTypes;

    u8 transportMaxDistance;
    EnumMap<TransportType, Array2<u8>> tileTransportDistance;
};

void initTransportLayer(TransportLayer* layer, City* city, MemoryArena* gameArena);
void updateTransportLayer(City* city, TransportLayer* layer);
void markTransportLayerDirty(TransportLayer* layer, Rect2I bounds);

void addTransportToTile(City* city, s32 x, s32 y, TransportType type);
void addTransportToTile(City* city, s32 x, s32 y, Flags<TransportType> types);
bool doesTileHaveTransport(City* city, s32 x, s32 y, TransportType type);
bool doesTileHaveTransport(City* city, s32 x, s32 y, Flags<TransportType> types);
s32 getDistanceToTransport(City* city, s32 x, s32 y, TransportType type);

void debugInspectTransport(UI::Panel* panel, City* city, s32 x, s32 y);

void saveTransportLayer(TransportLayer* layer, BinaryFileWriter* writer);
bool loadTransportLayer(TransportLayer* layer, City* city, BinaryFileReader* reader);
