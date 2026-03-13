/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Forward.h>
#include <Util/OwnPtr.h>

enum class AppStatus : u8 {
    MainMenu,
    Game,
    Credits,
    Quit,
};

class App {
public:
    // FIXME: Move SDL and window initialization into here. Maybe other things too.
    static NonnullOwnPtr<App> initialize(float seconds_per_frame, AppStatus initial_status);
    static App& the();
    ~App();

    float delta_time() const { return m_delta_time * m_speed_multiplier; }
    void set_delta_time(float delta_time) { m_delta_time = delta_time; }

    float speed_multiplier() const { return m_speed_multiplier; }
    void set_speed_multiplier(float multiplier) { m_speed_multiplier = multiplier; }

    Random& cosmetic_random() { return *m_cosmetic_random; }

    // FIXME: Replace these with a Scene class, which we hold a single one of.
    AppStatus app_status() const { return m_app_status; }
    void set_app_status(AppStatus status) { m_app_status = status; }
    GameState* game_state() const { return m_game_state; }
    void set_game_state(GameState* state) { m_game_state = state; }

private:
    explicit App(float seconds_per_frame, AppStatus);

    float m_delta_time { 0 };
    float m_speed_multiplier { 1 };

    NonnullOwnPtr<Random> m_cosmetic_random;
    AppStatus m_app_status;
    GameState* m_game_state;
};
