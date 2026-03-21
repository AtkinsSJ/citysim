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
#include <Sim/Sector.h>
#include <Util/Forward.h>

class CrimeLayer final : public Layer {
public:
    CrimeLayer() = default;
    CrimeLayer(City&, MemoryArena&);
    virtual ~CrimeLayer() override = default;

    virtual void update(City&) override;
    virtual void mark_dirty(Rect2I bounds) override;

    virtual void notify_new_building(BuildingDef const&, Building&) override;
    virtual void notify_building_demolished(BuildingDef const&, Building&) override;

    float get_police_coverage_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* police_buildings() { return &m_police_buildings; }
    Array2<u8>* tile_police_coverage() { return &m_tile_police_coverage; }

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;

private:
    DirtyRects m_dirty_rects;

    SectorGrid<BasicSector> m_sectors;

    Array2<u8> m_tile_police_coverage;

    ChunkedArray<BuildingRef> m_police_buildings;
    s32 m_total_jail_capacity;
    s32 m_occupied_jail_capacity;

    float m_funding_level; // @Budget
};
