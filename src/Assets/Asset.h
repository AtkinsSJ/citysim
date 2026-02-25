/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManager.h>
#include <Assets/Forward.h>

namespace Assets {

class Asset {
public:
    static AssetType asset_type() { VERIFY_NOT_REACHED(); }
    virtual ~Asset() = default;

    virtual void unload(AssetMetadata& metadata) = 0;
};

// Provides getters for the asset
#define ASSET_SUBCLASS_METHODS(T)                                                                                 \
private:                                                                                                          \
    inline static AssetType s_asset_type;                                                                         \
                                                                                                                  \
public:                                                                                                           \
    static T& get(StringView name)                                                                                \
    {                                                                                                             \
        return dynamic_cast<T&>(*getAsset(asset_type(), asset_manager().assetStrings.intern(name)).loaded_asset); \
    }                                                                                                             \
    static T& get_with_fallback(Optional<StringView> const& name, TypedAssetRef<T> const& fallback)               \
    {                                                                                                             \
        if (name.has_value())                                                                                     \
            return get(name.value());                                                                             \
        return fallback.get();                                                                                    \
    }                                                                                                             \
    static AssetType asset_type() { return s_asset_type; }                                                        \
    static void set_asset_type(AssetType type) { s_asset_type = type; }                                           \
    static TypedAssetRef<T> get_ref(StringView name)                                                              \
    {                                                                                                             \
        return TypedAssetRef<T> { asset_manager().assetStrings.intern(name) };                                    \
    }                                                                                                             \
                                                                                                                  \
private:

}

using Assets::Asset;
