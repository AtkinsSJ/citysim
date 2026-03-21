/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Sector.h>
#include <UI/Forward.h>
#include <Util/EnumMap.h>
#include <Util/Flags.h>

enum class TransportType : u8 {
    Road,
    Rail,
    COUNT
};

class TransportLayer {
public:
    TransportLayer() = default;
    TransportLayer(City&, MemoryArena&);

    void update(City&);
    void mark_dirty(Rect2I bounds);

    void add_transport_to_tile(s32 x, s32 y, TransportType);
    void add_transport_to_tile(s32 x, s32 y, Flags<TransportType>);

    bool tile_has_transport(s32 x, s32 y, TransportType) const;
    bool tile_has_transport(s32 x, s32 y, Flags<TransportType>) const;

    s32 distance_to_transport(s32 x, s32 y, TransportType) const;

    void debug_inspect(UI::Panel& panel, V2I tile_position);

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

private:
    u8 m_transport_max_distance { 8 };
    DirtyRects m_dirty_rects;

    Array2<Flags<TransportType>> m_tile_transport_types;

    EnumMap<TransportType, Array2<u8>> m_tile_transport_distance;
};
