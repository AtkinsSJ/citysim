/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

class AssetManagerListener {
public:
    virtual ~AssetManagerListener() = default;
    virtual void after_assets_loaded() { }
    virtual void before_assets_unloaded() { }
};
