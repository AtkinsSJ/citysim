/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "App.h"
#include <Util/Log.h>
#include <Util/Random.h>
#include <Util/String.h>

static App* s_app = nullptr;

NonnullOwnPtr<App> App::initialize(float seconds_per_frame, AppStatus initial_status)
{
    logInfo("Constructing app"_s);
    ASSERT(s_app == nullptr);
    s_app = new App(seconds_per_frame, initial_status);
    return adopt_own(*s_app);
}

App& App::the()
{
    ASSERT(s_app != nullptr);
    return *s_app;
}

App::App(float seconds_per_frame, AppStatus initial_status)
    : m_delta_time(seconds_per_frame)
    , m_cosmetic_random(adopt_own(*Random::create()))
    , m_app_status(initial_status)
{
}

App::~App()
{
    logInfo("Destructing app"_s);
    s_app = nullptr;
}
