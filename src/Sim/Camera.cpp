/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Camera.h"

#include <Debug/Debug.h>
#include <Gfx/Camera.h>
#include <Gfx/Renderer.h>
#include <Sim/Basic.h>

static void update_camera(PositionComponent const& position, CameraComponent& camera_component)
{
    DEBUG_FUNCTION();
    auto& camera = *camera_component.camera;
    camera.set_position(position.position);
    camera.set_zoom(camera_component.zoom);
    camera.update_projection_matrix();
}

mod_camera::mod_camera(flecs::world& world)
{
    world.module<mod_camera>();

    world.import<mod_basic>();

    world.component<CameraComponent>();

    world.system<PositionComponent const, CameraComponent>("UpdateCamera")
        .kind(flecs::PreStore)
        .each(update_camera);
}
