/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>

class Asset {
public:
    virtual ~Asset() = default;

    virtual void unload(AssetMetadata& metadata) = 0;
};
