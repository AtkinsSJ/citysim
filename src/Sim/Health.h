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
#include <Sim/Sector.h>

class HealthLayer {
public:
    HealthLayer() = default;
    HealthLayer(City&, MemoryArena&);

    void update(City&);
    void mark_dirty(Rect2I bounds);

    void notify_new_building(BuildingDef const&, Building&);
    void notify_building_demolished(BuildingDef const&, Building&);

    float get_health_coverage_percent_at(s32 x, s32 y) const;

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* health_buildings() { return &m_health_buildings; }
    Array2<u8>* tile_health_coverage() { return &m_tile_health_coverage; }

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

private:
    DirtyRects m_dirty_rects;

    SectorGrid<BasicSector> m_sectors;

    Array2<u8> m_tile_health_coverage;

    ChunkedArray<BuildingRef> m_health_buildings;

    float m_funding_level; // @Budget
};
