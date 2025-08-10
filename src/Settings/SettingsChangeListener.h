/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

class SettingsChangeListener {
public:
    virtual ~SettingsChangeListener() = default;
    virtual void on_settings_changed() = 0;
};
