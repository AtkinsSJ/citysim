/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "App.h"
#include <App/Scene.h>
#include <UI/Window.h>
#include <Util/Log.h>
#include <Util/Random.h>
#include <Util/String.h>

static App* s_app = nullptr;

NonnullOwnPtr<App> App::initialize(float seconds_per_frame, NonnullOwnPtr<Scene> scene)
{
    logInfo("Constructing app"_s);
    ASSERT(s_app == nullptr);
    s_app = new App(seconds_per_frame, move(scene));
    return adopt_own(*s_app);
}

App& App::the()
{
    ASSERT(s_app != nullptr);
    return *s_app;
}

App::App(float seconds_per_frame, NonnullOwnPtr<Scene> scene)
    : m_delta_time(seconds_per_frame)
    , m_cosmetic_random(Random::create())
    , m_scene(move(scene))
{
}

App::~App()
{
    logInfo("Destructing app"_s);
    // FIXME: Clear s_app? Currently we can't because this code runs before the members are destructed, and they might rely on App::the().
    //        (eg, GameScene's destructor does.)
}

void App::switch_to_scene(NonnullOwnPtr<Scene> scene)
{
    m_next_scene = move(scene);
}

void App::transition_to_next_scene_if_needed()
{
    if (!m_next_scene)
        return;
    UI::closeAllWindows();
    m_scene = m_next_scene.release_nonnull();
}

Scene& App::scene() const
{
    return *m_scene;
}
