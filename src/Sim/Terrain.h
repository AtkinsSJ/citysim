/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Sprite.h>
#include <IO/Forward.h>
#include <Sim/Forward.h>
#include <UI/Forward.h>
#include <Util/OccupancyArray.h>

using TerrainType = u8;

// FIXME: This isn't really a Layer. Rename it or move it into the City directly.
class TerrainLayer {
    // FIXME: For remapping terrain types. Maybe move that?
    friend TerrainCatalogue;

public:
    TerrainLayer() = default;
    TerrainLayer(City&, MemoryArena&);

    void generate(City&, u32 seed);
    u32 generation_seed() const { return m_terrain_generation_seed; }

    TerrainDef const& terrain_at(s32 x, s32 y) const;
    TerrainType terrain_type_at(s32 x, s32 y) const;
    void set_terrain_at(s32 x, s32 y, TerrainType);

    u8 height_at(s32 x, s32 y) const;

    u8 distance_to_water_at(s32 x, s32 y) const;

    void draw_terrain(Rect2I visible_area, s8 shader_id) const;

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

private:
    void update_distance_to_water(Rect2I bounds);
    void assign_terrain_sprites(Rect2I bounds);

    Rect2I m_bounds;

    s32 m_terrain_generation_seed { 0 };

    Array2<TerrainType> m_tile_terrain_type;
    Array2<u8> m_tile_height;
    Array2<u8> m_tile_distance_to_water;

    Array2<u8> m_tile_sprite_offset;
    Array2<SpriteRef> m_tile_sprite;
    Array2<Optional<SpriteRef>> m_tile_border_sprite;
};

void show_terrain_window();
void modify_terrain_window_proc(UI::WindowContext* context, void*);
