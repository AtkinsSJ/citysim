/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array2.h>
#include <Util/Basic.h>
#include <Util/Indexed.h>
#include <Util/Maths.h>
#include <Util/MemoryArena.h>
#include <Util/Rectangle.h>

struct BasicSector {
    Rect2I bounds;
};

template<typename Sector>
struct SectorGrid {
    SectorGrid() = default;
    SectorGrid(MemoryArena* arena, V2I world_size, s32 sector_size, s32 sectors_to_update_per_tick)
        : m_world_size(world_size)
        , m_sector_size(sector_size)
        , m_sectors(arena->allocate_array_2d<Sector>(divideCeil(world_size.x, sector_size), divideCeil(world_size.y, sector_size)))
        , m_sectors_to_update_per_tick(sectors_to_update_per_tick)
    {
        s32 remainder_width = world_size.x % sector_size;
        s32 remainder_height = world_size.y % sector_size;
        for (s32 y = 0; y < m_sectors.height(); y++) {
            for (s32 x = 0; x < m_sectors.width(); x++) {
                auto& sector = m_sectors.get(x, y);

                // FIXME: Do this properly.
                sector = {};
                sector.bounds = { x * sector_size, y * sector_size, sector_size, sector_size };

                if ((x == m_sectors.width() - 1) && remainder_width > 0) {
                    sector.bounds.set_width(remainder_width);
                }
                if ((y == m_sectors.height() - 1) && remainder_height > 0) {
                    sector.bounds.set_height(remainder_height);
                }
            }
        }
    }

    s32 sector_count() const
    {
        return m_sectors.count();
    }

    Sector& operator[](s32 index)
    {
        return m_sectors.get_flat(index);
    }

    Sector* get(s32 sector_x, s32 sector_y)
    {
        if (m_sectors.contains_coordinate(sector_x, sector_y))
            return &m_sectors.get(sector_x, sector_y);

        return nullptr;
    }

    Sector const* get(s32 sector_x, s32 sector_y) const
    {
        return const_cast<SectorGrid*>(this)->get(sector_x, sector_y);
    }

    Sector* get_sector_at_tile_pos(s32 x, s32 y)
    {
        Sector* result = nullptr;

        if (x >= 0 && x < m_world_size.x && y >= 0 && y < m_world_size.y) {
            result = get(x / m_sector_size, y / m_sector_size);
        }

        return result;
    }

    Sector const* get_sector_at_tile_pos(s32 x, s32 y) const
    {
        return const_cast<SectorGrid*>(this)->get_sector_at_tile_pos(x, y);
    }

    Sector* get_by_index(s32 index)
    {
        Sector* result = nullptr;

        if (index >= 0 && index < m_sectors.count()) {
            result = &m_sectors.get_flat(index);
        }

        return result;
    }

    Sector const* get_by_index(s32 index) const
    {
        return const_cast<SectorGrid*>(this)->get_by_index(index);
    }

    Rect2I get_sectors_covered(Rect2I area) const
    {
        auto intersected_area = area.intersected({ 0, 0, m_world_size.x, m_world_size.y });
        if (!intersected_area.has_positive_area())
            return {};

        return Rect2I::create_min_max(
            intersected_area.x() / m_sector_size,
            intersected_area.y() / m_sector_size,

            (intersected_area.x() + intersected_area.width() - 1) / m_sector_size,
            (intersected_area.y() + intersected_area.height() - 1) / m_sector_size);
    }

    Indexed<Sector> get_next_sector()
    {
        Indexed<Sector> result { m_next_sector_update_index, m_sectors.get_flat(m_next_sector_update_index) };

        m_next_sector_update_index = (m_next_sector_update_index + 1) % sector_count();

        return result;
    }

    s32 sectors_to_update_per_tick() const { return m_sectors_to_update_per_tick; }

private:
    V2I m_world_size;
    s32 m_sector_size;

    Array2<Sector> m_sectors;

    s32 m_next_sector_update_index { 0 };
    // FIXME: This is awkward being here. SectorGrid itself doesn't use it.
    s32 m_sectors_to_update_per_tick;
};
