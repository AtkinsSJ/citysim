/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/BuildingRef.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Layer.h>
#include <Sim/Sector.h>

class HealthLayer final : public Layer {
public:
    HealthLayer() = default;
    HealthLayer(City&, MemoryArena&);
    virtual ~HealthLayer() override = default;

    virtual void update(City&) override;
    virtual void mark_dirty(Rect2I bounds) override;

    virtual void notify_new_building(BuildingDef const&, Building&) override;
    virtual void notify_building_demolished(BuildingDef const&, Building&) override;

    float get_health_coverage_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* health_buildings() { return &m_health_buildings; }
    Array2<u8>* tile_health_coverage() { return &m_tile_health_coverage; }

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;

private:
    DirtyRects m_dirty_rects;

    SectorGrid<BasicSector> m_sectors;

    Array2<u8> m_tile_health_coverage;

    ChunkedArray<BuildingRef> m_health_buildings;

    float m_funding_level; // @Budget
};
