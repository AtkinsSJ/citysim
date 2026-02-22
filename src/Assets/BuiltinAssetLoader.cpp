/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "BuiltinAssetLoader.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Assets/Texts.h>
#include <Settings/Settings.h>

namespace Assets {

void BuiltinAssetLoader::register_types(AssetManager& assets)
{
    assets.directoryNameToType.put(assets.assetStrings.intern("locale"_s), AssetType::Texts);
    assets.asset_loaders_by_type[AssetType::Texts] = this;
    Texts::set_asset_type(AssetType::Texts);
}

void BuiltinAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(AssetType::Texts, adopt_own(*new Texts));
}

Optional<String> BuiltinAssetLoader::make_asset_path(AssetManager const& assets, AssetType type, StringView short_name)
{
    switch (type) {
    case AssetType::Texts:
        return myprintf("{0}/locale/{1}"_s, { assets.assetsPath, short_name }, true);
    default:
        return {};
    }
}

ErrorOr<NonnullOwnPtr<Asset>> BuiltinAssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == AssetType::Texts) {
        // Only load assets that match our locale
        // FIXME: Move locale checks into AssetManager
        if (metadata.locale == get_locale()) {
            return to_error_or_asset(Texts::load(metadata, file_data, false));
        }

        if (metadata.locale == Locale::En) {
            logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, { metadata.fullName, to_string(metadata.locale.value()), to_string(get_locale()) });
            return to_error_or_asset(Texts::load(metadata, file_data, true));
        }

        // Skip loading
        return "Not loading because it's the wrong locale."_s;
    }

    VERIFY_NOT_REACHED();
}

}
