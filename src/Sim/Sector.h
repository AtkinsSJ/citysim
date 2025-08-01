/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
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

    Sector& operator[](s32 index)
    {
        return this->sectors[index];
    }
};

struct BasicSector {
    Rect2I bounds;
};

// NB: The Sector struct needs to contain a "Rect2I bounds;" member. This is filled-in inside initSectorGrid().

template<typename Sector>
void initSectorGrid(SectorGrid<Sector>* grid, MemoryArena* arena, s32 cityWidth, s32 cityHeight, s32 sectorSize, s32 sectorsToUpdatePerTick = 0)
{
    grid->width = cityWidth;
    grid->height = cityHeight;
    grid->sectorSize = sectorSize;
    grid->sectorsX = divideCeil(cityWidth, sectorSize);
    grid->sectorsY = divideCeil(cityHeight, sectorSize);

    grid->nextSectorUpdateIndex = 0;
    grid->sectorsToUpdatePerTick = sectorsToUpdatePerTick;

    grid->sectors = arena->allocate_array<Sector>(grid->sectorsX * grid->sectorsY, true);

    s32 remainderWidth = cityWidth % sectorSize;
    s32 remainderHeight = cityHeight % sectorSize;
    for (s32 y = 0; y < grid->sectorsY; y++) {
        for (s32 x = 0; x < grid->sectorsX; x++) {
            Sector* sector = &grid->sectors[(grid->sectorsX * y) + x];

            *sector = {};
            sector->bounds = { x * sectorSize, y * sectorSize, sectorSize, sectorSize };

            if ((x == grid->sectorsX - 1) && remainderWidth > 0) {
                sector->bounds.w = remainderWidth;
            }
            if ((y == grid->sectorsY - 1) && remainderHeight > 0) {
                sector->bounds.h = remainderHeight;
            }
        }
    }
}

template<typename Sector>
Sector* getSector(SectorGrid<Sector>* grid, s32 sectorX, s32 sectorY)
{
    Sector* result = nullptr;

    if (sectorX >= 0 && sectorX < grid->sectorsX && sectorY >= 0 && sectorY < grid->sectorsY) {
        result = &grid->sectors[(sectorY * grid->sectorsX) + sectorX];
    }

    return result;
}

template<typename Sector>
Sector* getSectorByIndex(SectorGrid<Sector>* grid, s32 index)
{
    Sector* result = nullptr;

    if (index >= 0 && index < grid->sectors.count) {
        result = &grid->sectors[index];
    }

    return result;
}

template<typename Sector>
s32 getSectorCount(SectorGrid<Sector>* grid)
{
    return grid->sectors.count;
}

template<typename Sector>
Sector* getSectorAtTilePos(SectorGrid<Sector>* grid, s32 x, s32 y)
{
    Sector* result = nullptr;

    if (x >= 0 && x < grid->width && y >= 0 && y < grid->height) {
        result = getSector(grid, x / grid->sectorSize, y / grid->sectorSize);
    }

    return result;
}

template<typename Sector>
Rect2I getSectorsCovered(SectorGrid<Sector>* grid, Rect2I area)
{
    auto intersected_area = area.intersected({0, 0, grid->width, grid->height});

    return Rect2I::create_min_max(
        intersected_area.x / grid->sectorSize,
        intersected_area.y / grid->sectorSize,

        (intersected_area.x + intersected_area.w - 1) / grid->sectorSize,
        (intersected_area.y + intersected_area.h - 1) / grid->sectorSize);
}

template<typename Sector>
Sector* getNextSector(SectorGrid<Sector>* grid)
{
    Sector* result = &grid->sectors[grid->nextSectorUpdateIndex];

    grid->nextSectorUpdateIndex = (grid->nextSectorUpdateIndex + 1) % getSectorCount(grid);

    return result;
}

template<typename Sector>
Indexed<Sector*> getNextSectorWithIndex(SectorGrid<Sector>* grid)
{
    Indexed<Sector*> result = makeIndexedValue(&grid->sectors[grid->nextSectorUpdateIndex], grid->nextSectorUpdateIndex);

    grid->nextSectorUpdateIndex = (grid->nextSectorUpdateIndex + 1) % getSectorCount(grid);

    return result;
}
