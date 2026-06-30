/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MapGeneration.h"

#include <App/App.h>
#include <Sim/Basic.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/TerrainCatalogue.h>
#include <Util/Random.h>
#include <Util/Splat.h>

struct MapGenTag { };
flecs::entity mod_map_generation::map_generation_pipeline;

static void generate_map_impl(flecs::iter& it, size_t, MapData const& map_data, BuildingAtPosition const& building_at_position, TerrainData& terrain)
{
    DEBUG_FUNCTION();
    logInfo("Generate map!"_s);

    auto world = it.world();
    world.defer_suspend();
    Deferred resume_suspend = [&] { world.defer_resume(); };

    auto& bounds = map_data.bounds;
    auto& cosmetic_random = App::the().cosmetic_random();
    auto& building_catalogue = BuildingCatalogue::the();

    u8 ground_tile = truncate<u8>(findTerrainTypeByName("ground"_s));
    u8 water_tile = truncate<u8>(findTerrainTypeByName("water"_s));

    auto terrain_random = Random::create(map_data.generation_seed);
    terrain.tile_terrain_type.fill(ground_tile);

    for (s32 y = 0; y < bounds.height(); y++) {
        for (s32 x = 0; x < bounds.width(); x++) {
            terrain.tile_sprite_offset.set(x, y, cosmetic_random.random_integer<u8>());
        }
    }

    // Generate a river
    Array<float> river_offset = temp_arena().allocate_filled_array<float>(bounds.height());
    terrain_random->fill_with_noise(river_offset, 10);
    float river_max_width = terrain_random->random_float_between(12, 16);
    float river_min_width = terrain_random->random_float_between(6, river_max_width);
    float river_waviness = 16.0f;
    s32 river_centre_base = terrain_random->random_between(ceil_s32(bounds.width() * 0.4f), floor_s32(bounds.width() * 0.6f));
    for (s32 y = 0; y < bounds.height(); y++) {
        s32 river_width = ceil_s32(lerp(river_min_width, river_max_width, ((float)y / (float)bounds.height())));
        s32 river_centre = river_centre_base - round_s32((river_waviness * 0.5f) + (river_waviness * river_offset[y]));
        s32 river_left = river_centre - (river_width / 2);

        for (s32 x = river_left; x < river_left + river_width; x++) {
            terrain.tile_terrain_type.set(x, y, water_tile);
        }
    }

    // Coastline
    Array<float> coastline_offset = temp_arena().allocate_filled_array<float>(bounds.width());
    terrain_random->fill_with_noise(coastline_offset, 10);
    for (s32 x = 0; x < bounds.width(); x++) {
        s32 coast_depth = 8 + round_s32(coastline_offset[x] * 16.0f);

        for (s32 i = 0; i < coast_depth; i++) {
            s32 y = bounds.height() - 1 - i;

            terrain.tile_terrain_type.set(x, y, water_tile);
        }
    }

    // Lakes/ponds
    s32 pond_count = terrain_random->random_between(1, 4);
    for (s32 pond_index = 0; pond_index < pond_count; pond_index++) {
        s32 pond_centre_x = terrain_random->random_between(0, bounds.width());
        s32 pond_centre_y = terrain_random->random_between(0, bounds.height());

        float pond_min_radius = terrain_random->random_float_between(3.0f, 5.0f);
        float pond_max_radius = terrain_random->random_float_between(pond_min_radius + 3.0f, 20.0f);

        Splat pond_splat = Splat::create_random(pond_centre_x, pond_centre_y, pond_min_radius, pond_max_radius, 36, terrain_random);
        pond_splat.for_each_tile([&](s32 x, s32 y) {
            if (!bounds.contains(x, y))
                return;
            terrain.tile_terrain_type.set(x, y, water_tile);
        });
    }

    // Forest splats
    auto tree_def = building_catalogue.try_find_def("tree"_s);
    if (!tree_def.has_value()) {
        logError("Map generator is unable to place any trees, because the 'tree' building was not found."_s);
    } else {
        s32 forest_count = terrain_random->random_between(10, 20);
        for (s32 forest_index = 0; forest_index < forest_count; forest_index++) {
            s32 centre_x = terrain_random->random_between(0, bounds.width());
            s32 centre_y = terrain_random->random_between(0, bounds.height());

            float min_radius = terrain_random->random_float_between(2.0f, 5.0f);
            float max_radius = terrain_random->random_float_between(min_radius + 1.0f, 15.0f);

            Splat forest_splat = Splat::create_random(centre_x, centre_y, min_radius, max_radius, 36, terrain_random);
            forest_splat.for_each_tile_with_centre_distance([&](s32 x, s32 y, float percentage_distance_from_centre) {
                if (!bounds.contains(x, y))
                    return;

                // Make forests less dense towards their edges
                float density = sqrt_float(percentage_distance_from_centre);
                if (terrain_random->random_float_0_1() > density)
                    return;

                if (TerrainCatalogue::the().get_def(terrain.tile_terrain_type.get(x, y)).canBuildOn
                    && !building_at_position.tile_building.get_if_exists(x, y, {}).has_value()) {

                    tree_def->instantiate(world, v2i(x, y));
                }
            });
        }
    }
}

mod_map_generation::mod_map_generation(flecs::world& world)
{
    world.module<mod_map_generation>();

    world.import<mod_basic>();
    world.import<mod_building>();
    world.import<mod_terrain>();

    // Build the pipeline
    auto the_enum = enum_type<MapGenPhase>(world);
    auto deallocate_phase = world.entity(the_enum.entity(MapGenPhase::Deallocate)).add<MapGenTag>();
    auto allocate_phase = world.entity(the_enum.entity(MapGenPhase::Allocate)).add<MapGenTag>().depends_on(deallocate_phase);
    auto on_phase = world.entity(the_enum.entity(MapGenPhase::Generate)).add<MapGenTag>().depends_on(allocate_phase);
    [[maybe_unused]] auto post_phase = world.entity(the_enum.entity(MapGenPhase::Post)).add<MapGenTag>().depends_on(on_phase);
    map_generation_pipeline = world.pipeline()
                                  .with(flecs::System)
                                  .with<MapGenTag>()
                                  .cascade(flecs::DependsOn)
                                  .without(flecs::Disabled)
                                  .up(flecs::DependsOn)
                                  .without(flecs::Disabled)
                                  .up(flecs::ChildOf)
                                  .build();

    world.system<MapData const, BuildingAtPosition const, TerrainData>("MapGen")
        .kind(MapGenPhase::Generate)
        .read<MapData>()
        .read<BuildingAtPosition>()
        .with<TerrainData>()
        .read_write()
        .immediate()
        .each(generate_map_impl);
}

void generate_map(flecs::world& world, u32 seed)
{
    world.set<MapData>({
        .generation_seed = seed,
        .bounds = { 0, 0, 128, 128 },
    });

    ASSERT(mod_map_generation::map_generation_pipeline.is_valid());
    world.run_pipeline(mod_map_generation::map_generation_pipeline);
}
