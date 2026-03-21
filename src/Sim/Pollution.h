/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Layer.h>

class PollutionLayer final : public Layer {
public:
    PollutionLayer() = default;
    PollutionLayer(City&, MemoryArena&);
    virtual ~PollutionLayer() override = default;

    virtual void update(City&) override;
    virtual void mark_dirty(Rect2I bounds) override;

    float get_pollution_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    Array2<u8>* tile_pollution() { return &m_tile_pollution; }

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;

private:
    DirtyRects m_dirty_rects;

    Array2<s16> m_tile_building_contributions;

    Array2<u8> m_tile_pollution; // Cached total
};

s32 const maxPollutionEffectDistance = 16; // TODO: Better value for this!
