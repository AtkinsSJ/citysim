/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetLoader.h>

namespace Gfx {

class AssetLoader final : public Assets::AssetLoader {
public:
    virtual ~AssetLoader() override = default;

    virtual void register_types(AssetManager&) override;
    virtual void create_placeholder_assets(AssetManager&) override;
    virtual Optional<String> make_asset_path(AssetManager const&, AssetType, StringView short_name) override;

    virtual ErrorOr<NonnullOwnPtr<Asset>> load_asset(AssetMetadata&, Blob file_data) override;

private:
    AssetType m_cursor_defs_type;
    AssetType m_palette_defs_type;
    AssetType m_sprite_defs_type;
};

}
