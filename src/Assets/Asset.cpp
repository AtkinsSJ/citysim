/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Asset.h"
#include <Assets/AssetManager.h>

void Asset::ensure_is_loaded()
{
    if (state == State::Loaded)
        return;

    loadAsset(this);

    if (state != State::Loaded) {
        logError("Failed to load asset '{0}'"_s, { shortName });
    }
}
