/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetLoader.h"
#include <Assets/AssetManager.h>
#include <Sim/BuildingDefs.h>
#include <Sim/TerrainDefs.h>
#include <Util/Blob.h>

namespace Sim {

void AssetLoader::register_types(AssetManager& assets)
{
    BuildingDefs::set_asset_type(assets.register_asset_type("BuildingDefs"_s, *this, { .file_extension = "buildings"_sv }));
    TerrainDefs::set_asset_type(assets.register_asset_type("TerrainDefs"_s, *this, { .file_extension = "terrain"_sv }));
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(BuildingDefs::asset_type(), adopt_own(*new BuildingDefs));
    assets.set_placeholder_asset(TerrainDefs::asset_type(), adopt_own(*new TerrainDefs));
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == BuildingDefs::asset_type())
        return to_error_or_asset(BuildingDefs::load(metadata, file_data));

    if (metadata.type == TerrainDefs::asset_type())
        return to_error_or_asset(TerrainDefs::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}

}
