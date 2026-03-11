/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetRef.h>
#include <Assets/Forward.h>
#include <Util/Flags.h>
#include <Util/Locale.h>
#include <Util/OwnPtr.h>

namespace Assets {

enum class AssetFlags : u8 {
    IsAFile, // The file at Asset.fullName should be loaded into memory when loading this Asset

    COUNT,
};

constexpr Flags<AssetFlags> default_asset_flags { AssetFlags::IsAFile };

class AssetMetadata {
public:
    void ensure_is_loaded();

    template<typename T>
    TypedAssetRef<T> get_ref() const
    {
        auto ref = T::get_ref(shortName);
        ASSERT(ref.type() == type);
        return ref;
    }

    GenericAssetRef get_ref() const
    {
        return { shortName, type };
    }

    AssetType type;

    // shortName = "foo.png", fullName = "c:/mygame/assets/textures/foo.png"
    String shortName;
    String fullName;

    Flags<AssetFlags> flags;

    enum class State : u8 {
        Unloaded,
        Loaded,
        Error,
    };
    State state { State::Unloaded };
    Optional<Locale> locale;

    OwnPtr<Asset> loaded_asset;
};

}

using Assets::AssetMetadata;
