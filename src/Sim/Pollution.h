/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>

class PollutionLayer {
public:
    PollutionLayer() = default;
    PollutionLayer(City&, MemoryArena&);

    void update(City&);
    void mark_dirty(Rect2I bounds);

    float get_pollution_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    Array2<u8>* tile_pollution() { return &m_tile_pollution; }

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

private:
    DirtyRects m_dirty_rects;

    Array2<s16> m_tile_building_contributions;

    Array2<u8> m_tile_pollution; // Cached total
};

s32 const maxPollutionEffectDistance = 16; // TODO: Better value for this!
