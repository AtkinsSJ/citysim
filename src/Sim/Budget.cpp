/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Budget.h"

#include <Sim/Game.h>
#include <Sim/MapGeneration.h>
#include <UI/UI.h>
#include <UI/Window.h>

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

static void cost_tooltip_window_proc(UI::WindowContext* context, void* userData)
{
    auto* game_scene = dynamic_cast<GameScene*>(&App::the().scene());
    if (!game_scene) {
        context->closeRequested = true;
        return;
    }
    auto& ui = context->windowPanel;
    s32 cost = truncate32(reinterpret_cast<smm>(userData));

    auto& budget = game_scene->world().get<Budget>();
    auto style = budget.can_afford(cost)
        ? "cost-affordable"_sv
        : "cost-unaffordable"_sv;

    String text = myprintf("£{0}"_s, { formatInt(cost) });
    ui.addLabel(text, style);
}

void Budget::show_cost_tooltip(s32 cost) const
{
    UI::showTooltip(cost_tooltip_window_proc, reinterpret_cast<void*>(static_cast<smm>(cost)));
}
