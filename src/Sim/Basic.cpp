/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Basic.h"
#include <Gfx/Renderer.h>
#include <Sim/City.h>

static void update_visible_tile_bounds(flecs::iter& it)
{
    // Pre-calculate the tile area that's visible to the player.
    // We err on the side of drawing too much, rather than risking having holes in the world.
    auto& renderer = the_renderer();
    auto world_camera = renderer.world_camera();
    auto world = it.world();
    auto& city_bounds = world.get<CityData>().bounds;

    world.set<VisibleTileBounds>({
        Rect2I::create_centre_size(
            v2i(world_camera.position()),
            v2i(world_camera.size() / world_camera.zoom()) + v2i(3, 3))
            .intersected(city_bounds),
    });
}

static void draw_entities(flecs::iter& it)
{
    Rect2I const& crop_area = it.world().get<VisibleTileBounds>().rect;
    auto& renderer = the_renderer();
    auto shader_id = renderer.shaderIds.pixelArt;
    auto world_buffer = renderer.world_buffer();

    // FIXME: Demolition tinting - make that generic instead of hard coded here!
    // bool isDemolitionHappening = false; // demolitionRect.has_positive_area();
    // auto drawColorDemolish = Colour::from_rgb_255(255, 128, 128, 255);

    while (it.next()) {
        auto positions = it.field<PositionComponent const>(0);
        auto sprites = it.field<SpriteComponent const>(0);

        for (auto i : it) {
            auto& position = positions[i];
            auto& sprite = sprites[i];
            Rect2 sprite_bounds = { position.position, sprite.size };
            if (!sprite_bounds.overlaps(crop_area))
                continue;

            drawSingleSprite(&world_buffer, &sprite.sprite.get(), sprite_bounds, shader_id, sprite.color);
        }
    }
}

mod_basic::mod_basic(flecs::world& world)
{
    world.module<mod_basic>();

    world.component<PositionComponent>();
    world.component<SpriteComponent>();

    world.component<VisibleTileBounds>().add(flecs::Singleton);

    world.system("UpdateVisibleTileBounds")
        .kind(flecs::PreStore)
        .run(update_visible_tile_bounds);

    // FIXME: Depth sorting
    world.system<PositionComponent const, SpriteComponent const>("DrawEntities")
        .kind(flecs::OnStore)
        .run(draw_entities);
}
