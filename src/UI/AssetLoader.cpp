/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetLoader.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Assets/ContainerAsset.h>
#include <UI/UITheme.h>

namespace UI {

void AssetLoader::register_types(AssetManager& assets)
{
    m_theme_asset_type = assets.register_asset_type("UITheme"_s, *this, { .file_extension = "theme"_sv });

    ButtonStyle::set_asset_type(assets.register_asset_type("ButtonStyle"_s, *this));
    CheckboxStyle::set_asset_type(assets.register_asset_type("CheckboxStyle"_s, *this));
    ConsoleStyle::set_asset_type(assets.register_asset_type("ConsoleStyle"_s, *this));
    DropDownListStyle::set_asset_type(assets.register_asset_type("DropDownListStyle"_s, *this));
    LabelStyle::set_asset_type(assets.register_asset_type("LabelStyle"_s, *this));
    PanelStyle::set_asset_type(assets.register_asset_type("PanelStyle"_s, *this));
    RadioButtonStyle::set_asset_type(assets.register_asset_type("RadioButtonStyle"_s, *this));
    ScrollbarStyle::set_asset_type(assets.register_asset_type("ScrollbarStyle"_s, *this));
    SliderStyle::set_asset_type(assets.register_asset_type("SliderStyle"_s, *this));
    TextInputStyle::set_asset_type(assets.register_asset_type("TextInputStyle"_s, *this));
    WindowStyle::set_asset_type(assets.register_asset_type("WindowStyle"_s, *this));
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(m_theme_asset_type, adopt_own(*new ContainerAsset));
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    if (metadata.type == m_theme_asset_type)
        return load_theme(metadata, file_data);

    VERIFY_NOT_REACHED();
}

}
