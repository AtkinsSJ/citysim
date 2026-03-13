/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <App/Scene.h>

class MainMenuScene final : public Scene {
public:
    static NonnullOwnPtr<MainMenuScene> create();

    virtual ~MainMenuScene() override = default;
    virtual void update_and_render(float delta_time) override;
};
