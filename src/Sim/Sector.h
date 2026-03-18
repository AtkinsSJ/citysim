/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Basic.h>
#include <Util/Indexed.h>
#include <Util/Maths.h>
#include <Util/MemoryArena.h>
#include <Util/Rectangle.h>

template<typename Sector>
struct SectorGrid {
    s32 width, height; // world size
    s32 sectorSize;
    s32 sectorsX, sectorsY;

    Array<Sector> sectors;

    s32 nextSectorUpdateIndex;
    s32 sectorsToUpdatePerTick;

    s32 sector_count() const
    {
        return sectors.count;
    }

    Sector& operator[](s32 index)
    {
        return this->sectors[index];
    }

    Sector* get(s32 sector_x, s32 sector_y)
    {
        Sector* result = nullptr;

        if (sector_x >= 0 && sector_x < sectorsX && sector_y >= 0 && sector_y < sectorsY) {
            result = &sectors[(sector_y * sectorsX) + sector_x];
        }

        return result;
    }

    Sector const* get(s32 sector_x, s32 sector_y) const
    {
        return const_cast<SectorGrid*>(this)->get(sector_x, sector_y);
    }

    Sector* get_sector_at_tile_pos(s32 x, s32 y)
    {
        Sector* result = nullptr;

        if (x >= 0 && x < width && y >= 0 && y < height) {
            result = get(x / sectorSize, y / sectorSize);
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

        if (index >= 0 && index < sectors.count) {
            result = &sectors[index];
        }

        return result;
    }

    Sector const* get_by_index(s32 index) const
    {
        return const_cast<SectorGrid*>(this)->get_by_index(index);
    }

    Rect2I get_sectors_covered(Rect2I area) const
    {
        auto intersected_area = area.intersected({ 0, 0, width, height });
        if (!intersected_area.has_positive_area())
            return {};

        return Rect2I::create_min_max(
            intersected_area.x() / sectorSize,
            intersected_area.y() / sectorSize,

            (intersected_area.x() + intersected_area.width() - 1) / sectorSize,
            (intersected_area.y() + intersected_area.height() - 1) / sectorSize);
    }

    Indexed<Sector> get_next_sector()
    {
        Indexed<Sector> result { nextSectorUpdateIndex, sectors[nextSectorUpdateIndex] };

        nextSectorUpdateIndex = (nextSectorUpdateIndex + 1) % sector_count();

        return result;
    }
};

struct BasicSector {
    Rect2I bounds;
};

// NB: The Sector struct needs to contain a "Rect2I bounds;" member. This is filled-in inside initSectorGrid().

template<typename Sector>
void initSectorGrid(SectorGrid<Sector>* grid, MemoryArena* arena, V2I city_size, s32 sectorSize, s32 sectorsToUpdatePerTick = 0)
{
    grid->width = city_size.x;
    grid->height = city_size.y;
    grid->sectorSize = sectorSize;
    grid->sectorsX = divideCeil(city_size.x, sectorSize);
    grid->sectorsY = divideCeil(city_size.y, sectorSize);

    grid->nextSectorUpdateIndex = 0;
    grid->sectorsToUpdatePerTick = sectorsToUpdatePerTick;

    grid->sectors = arena->allocate_array<Sector>(grid->sectorsX * grid->sectorsY, true);

    s32 remainderWidth = city_size.x % sectorSize;
    s32 remainderHeight = city_size.y % sectorSize;
    for (s32 y = 0; y < grid->sectorsY; y++) {
        for (s32 x = 0; x < grid->sectorsX; x++) {
            Sector* sector = &grid->sectors[(grid->sectorsX * y) + x];

            *sector = {};
            sector->bounds = { x * sectorSize, y * sectorSize, sectorSize, sectorSize };

            if ((x == grid->sectorsX - 1) && remainderWidth > 0) {
                sector->bounds.set_width(remainderWidth);
            }
            if ((y == grid->sectorsY - 1) && remainderHeight > 0) {
                sector->bounds.set_height(remainderHeight);
            }
        }
    }
}
