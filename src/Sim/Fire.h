/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/GameClock.h>
#include <Sim/Sector.h>
#include <UI/Forward.h>
#include <Util/ChunkedArray.h>
#include <Util/Forward.h>
#include <Util/Rectangle.h>
#include <Util/Vector.h>

struct Fire {
    Entity* entity;

    V2I pos;
    // TODO: severity
    // TODO: Timing information (fires shouldn't last forever!)
    GameTimestamp startDate;
};

struct FireSector {
    Rect2I bounds;

    ChunkedArray<Fire> activeFires;
};

class FireLayer {
public:
    FireLayer() = default;
    FireLayer(City&, MemoryArena&);

    void update(City&);
    void mark_dirty(Rect2I bounds);

    Optional<Indexed<Fire>> find_fire_at(s32 x, s32 y);
    bool does_area_contain_fire(Rect2I bounds) const;
    void start_fire_at(City&, s32 x, s32 y);
    void add_fire_raw(City&, s32 x, s32 y, GameTimestamp start_date);
    void remove_fire_at(City&, s32 x, s32 y);

    u8 get_fire_risk_at(s32 x, s32 y) const;
    float get_fire_protection_percent_at(s32 x, s32 y) const;

    void notify_new_building(BuildingDef const&, Building&);
    void notify_building_demolished(BuildingDef const&, Building&);

    void debug_inspect(UI::Panel& panel, V2I tile_position, Building*);

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* fire_protection_buildings() { return &m_fire_protection_buildings; }
    Array2<u8>* tile_overall_fire_risk() { return &m_tile_overall_fire_risk; }

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&, City&);

private:
    DirtyRects m_dirty_rects;

    SectorGrid<FireSector> m_sectors;

    u8 m_max_fire_radius;
    Array2<u16> m_tile_fire_proximity_effect;

    Array2<u8> m_tile_total_fire_risk; // Risks combined

    Array2<u8> m_tile_fire_protection;

    Array2<u8> m_tile_overall_fire_risk; // Risks after we've taken protection into account

    ChunkedArray<BuildingRef> m_fire_protection_buildings;

    ArrayChunkPool<Fire> m_fire_pool;
    s32 m_active_fire_count;

    float m_funding_level; // @Budget
};
