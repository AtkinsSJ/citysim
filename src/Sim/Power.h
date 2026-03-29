/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Building.h>
#include <Sim/DirtyRects.h>
#include <Sim/Layer.h>
#include <Sim/Sector.h>

struct PowerGroup {
    s32 production;
    s32 consumption;

    // TODO: @Size These are always either 1-wide or 1-tall, and up to sectorSize in the other direction, so we could use a much smaller struct than Rect2I!
    ChunkedArray<Rect2I> sectorBoundaries; // Places in nighbouring sectors that are adjacent to this PowerGroup
    s32 networkID;

    ChunkedArray<BuildingRef> buildings;
};

struct PowerSector : public BasicSector {
    u8 get_power_group_id(s32 relX, s32 relY) const;
    void set_power_group_id(s32 relX, s32 relY, u8 value);
    PowerGroup* get_power_group_at(s32 relX, s32 relY);

    void update_power_values(City&);

    void flood_fill_power_group(s32 x, s32 y, u8 fillValue);
    void set_rect_power_group_unknown(Rect2I area);

    // 0 = none, >0 = any tile with the same value is connected
    // POWER_GROUP_UNKNOWN is used as a temporary value while recalculating
    Array2<u8> tilePowerGroup;

    // NB: Power groups start at 1, (0 means "none") so subtract 1 from the value in tilePowerGroup to get the index!
    ChunkedArray<PowerGroup> powerGroups;
};

struct PowerNetwork {
    s32 id;

    ChunkedArray<PowerGroup*> groups;

    s32 cachedProduction { 0 };
    s32 cachedConsumption { 0 };
};

class PowerLayer final : public Layer {
public:
    PowerLayer() = default;
    PowerLayer(City&, MemoryArena&);
    virtual ~PowerLayer() override = default;

    virtual void update(City&) override;
    virtual void mark_dirty(Rect2I bounds) override;

    virtual void notify_new_building(BuildingDef const&, Building&) override;
    virtual void notify_building_demolished(BuildingDef const&, Building&) override;

    bool does_tile_have_power_network(s32 x, s32 y) const;
    u8 get_distance_to_power(s32 x, s32 y) const;

    u8 calculate_power_overlay_for_tile(s32 x, s32 y) const;

    void debug_inspect(UI::Panel& panel, V2I tile_position);

    // FIXME: Temporary
    ChunkedArray<BuildingRef>* power_buildings() { return &m_power_buildings; }

    // FIXME: We should probably save and load this?
    virtual void save(BinaryFileWriter&) const override { }
    virtual bool load(BinaryFileReader&, City&) override { return true; }

private:
    PowerNetwork& new_power_network();
    void free_power_network(PowerNetwork&);
    PowerNetwork const* get_power_network_at(s32 x, s32 y) const;

    void recalculate_sector_power_groups(City&, PowerSector&);
    void flood_fill_city_power_network(PowerGroup&, PowerNetwork&);
    void recalculate_power_connectivity();

    Rect2I m_bounds;

    u8 m_power_max_distance { 2 };
    DirtyRects m_dirty_rects;

    SectorGrid<PowerSector> m_sectors;

    Array2<u8> m_tile_power_distance;

    ChunkedArray<PowerNetwork> m_networks;

    ArrayChunkPool<PowerGroup> m_power_groups_chunk_pool;
    ArrayChunkPool<PowerGroup*> m_power_group_pointers_chunk_pool;

    ChunkedArray<BuildingRef> m_power_buildings;

    s32 m_cached_combined_production { 0 };
    s32 m_cached_combined_consumption { 0 };
};

u8 const POWER_GROUP_UNKNOWN = 255;
