/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <App/App.h>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void update_and_render(float delta_time) = 0;
};
