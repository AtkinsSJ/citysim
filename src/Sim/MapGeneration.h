/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <flecs.h>

struct mod_map_generation {
    explicit mod_map_generation(flecs::world&);

    static flecs::entity map_generation_pipeline;
};

enum class MapGenPhase : u8 {
    Deallocate,
    Allocate,
    Generate,
    Post,
};

void generate_map(flecs::world&, u32 seed);
