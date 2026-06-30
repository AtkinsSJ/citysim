/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Budget.h"

#include <Sim/MapGeneration.h>

mod_budget::mod_budget(flecs::world& world)
{
    world.module<mod_budget>();

    world.component<Budget>().add(flecs::Singleton);

    // FIXME: When should we assign the starting funds?
    world.system()
        .write<Budget>()
        .kind(MapGenPhase::Post)
        .each([](flecs::iter& it, size_t) {
            auto world = it.world();
            s32 game_start_funds = 1'000'000;
            world.emplace<Budget>(game_start_funds);
        });
}

Budget::Budget(s32 funds)
    : m_funds(funds)
{
}

bool Budget::can_afford(s32 cost) const
{
    return m_funds >= cost;
}

void Budget::spend(s32 cost)
{
    m_funds -= cost;
}
