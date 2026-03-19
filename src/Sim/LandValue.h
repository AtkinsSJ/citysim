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

class LandValueLayer {
public:
    LandValueLayer() = default;
    LandValueLayer(City&, MemoryArena&);

    void update(City&);
    void mark_dirty(Rect2I bounds);

    float get_land_value_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    Array2<u8>* tile_land_value() { return &m_tile_land_value; }

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

private:
    DirtyRects m_dirty_rects;
    SectorGrid<BasicSector> m_sectors;
    Array2<s16> m_tile_building_contributions;
    Array2<u8> m_tile_land_value; // Cached total
};

s32 const maxLandValueEffectDistance = 16; // TODO: Better value for this!
