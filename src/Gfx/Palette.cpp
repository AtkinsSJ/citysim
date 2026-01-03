/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "Palette.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetType.h>

Palette& Palette::get(StringView name)
{
    return getAsset(AssetType::Palette, name.deprecated_to_string()).palette;
}
