/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <App/Forward.h>
#include <Util/OwnedPtr.h>

class App {
public:
    // FIXME: Move SDL and window initialization into here. Maybe other things too.
    static OwnedRef<App> initialize(float seconds_per_frame, OwnedRef<Scene>);
    static App& the();
    ~App();

    float delta_time() const { return m_delta_time * m_speed_multiplier; }
    void set_delta_time(float delta_time) { m_delta_time = delta_time; }

    float speed_multiplier() const { return m_speed_multiplier; }
    void set_speed_multiplier(float multiplier) { m_speed_multiplier = multiplier; }

    Random& cosmetic_random() { return *m_cosmetic_random; }

    void switch_to_scene(OwnedRef<Scene>);
    void transition_to_next_scene_if_needed();
    Scene& scene() const;

private:
    explicit App(float seconds_per_frame, OwnedRef<Scene>);

    float m_delta_time { 0 };
    float m_speed_multiplier { 1 };

    OwnedRef<Random> m_cosmetic_random;
    OwnedRef<Scene> m_scene;
    OwnedPtr<Scene> m_next_scene { nullptr };
};
