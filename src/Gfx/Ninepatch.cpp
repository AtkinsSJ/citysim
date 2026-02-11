/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Ninepatch.h"
#include <Assets/AssetManager.h>
#include <Util/OwnPtr.h>

NonnullOwnPtr<Ninepatch> Ninepatch::make_placeholder()
{
    auto ninepatch = adopt_own(*new Ninepatch);
    ninepatch->texture = &asset_manager().placeholderAssets[AssetType::Texture];
    return ninepatch;
}

void Ninepatch::unload(AssetMetadata&)
{
    // No-op
}
