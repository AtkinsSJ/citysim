/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetType.h>
#include <Assets/Forward.h>
#include <Util/String.h>

struct AssetRef {
    AssetType type;
    String name;

    Asset* pointer;
    u32 pointerRetrievedTicks;

    bool isValid()
    {
        return !name.is_empty();
    }
};
