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
#include <Sim/Layer.h>
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

struct FireSector : public BasicSector {
    ChunkedArray<Fire> activeFires;
};

class FireLayer final : public Layer {
public:
    FireLayer() = default;
    FireLayer(City&, MemoryArena&);
    virtual ~FireLayer() override = default;

    virtual void update(City&) override;
    virtual void mark_dirty(Rect2I bounds) override;

    Optional<Indexed<Fire>> find_fire_at(s32 x, s32 y);
    bool does_area_contain_fire(Rect2I bounds) const;
    void start_fire_at(City&, s32 x, s32 y);
    void add_fire_raw(City&, s32 x, s32 y, GameTimestamp start_date);
    void remove_fire_at(City&, s32 x, s32 y);

    u8 get_fire_risk_at(s32 x, s32 y) const;
    float get_fire_protection_percent_at(s32 x, s32 y) const;

    virtual void notify_new_building(BuildingDef const&, Building&) override;
    virtual void notify_building_demolished(BuildingDef const&, Building&) override;

    void debug_inspect(UI::Panel& panel, V2I tile_position, Building*);

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* fire_protection_buildings() { return &m_fire_protection_buildings; }
    Array2<u8>* tile_overall_fire_risk() { return &m_tile_overall_fire_risk; }

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;

private:
    u8 m_max_fire_radius { 4 };
    DirtyRects m_dirty_rects;

    SectorGrid<FireSector> m_sectors;

    Array2<u16> m_tile_fire_proximity_effect;

    Array2<u8> m_tile_total_fire_risk; // Risks combined

    Array2<u8> m_tile_fire_protection;

    Array2<u8> m_tile_overall_fire_risk; // Risks after we've taken protection into account

    ChunkedArray<BuildingRef> m_fire_protection_buildings;

    ArrayChunkPool<Fire> m_fire_pool;
    s32 m_active_fire_count;

    float m_funding_level; // @Budget
};
