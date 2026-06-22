/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Forward.h>
#include <Util/Ref.h>
#include <flecs.h>

struct mod_camera {
    explicit mod_camera(flecs::world&);
};

struct CameraComponent {
    Ref<Camera> camera;
    float zoom;
};
