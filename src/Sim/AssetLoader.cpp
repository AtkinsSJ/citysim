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
    assets.fileExtensionToType.put(assets.assetStrings.intern("buildings"_s), AssetType::BuildingDefs);
    assets.asset_loaders_by_type[AssetType::BuildingDefs] = this;
    BuildingDefs::set_asset_type(AssetType::BuildingDefs);

    assets.fileExtensionToType.put(assets.assetStrings.intern("terrain"_s), AssetType::TerrainDefs);
    assets.asset_loaders_by_type[AssetType::TerrainDefs] = this;
    TerrainDefs::set_asset_type(AssetType::TerrainDefs);
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(AssetType::BuildingDefs, adopt_own(*new BuildingDefs));
    assets.set_placeholder_asset(AssetType::TerrainDefs, adopt_own(*new TerrainDefs));
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == AssetType::BuildingDefs)
        return to_error_or_asset(BuildingDefs::load(metadata, file_data));

    if (metadata.type == AssetType::TerrainDefs)
        return to_error_or_asset(TerrainDefs::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}

}
