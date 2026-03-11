/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Util/String.h>

namespace Assets {

class AssetRef {
public:
    explicit AssetRef(String short_name)
        : m_name(move(short_name))
    {
    }
    virtual ~AssetRef() = default;

    Asset& asset() const;
    AssetMetadata& metadata() const;
    String const& name() const { return m_name; }
    virtual AssetType type() const = 0;

protected:
    String m_name;
    mutable AssetMetadata* m_pointer { nullptr };
    mutable u32 m_asset_generation { 0 };
};

template<typename T>
class TypedAssetRef : public AssetRef {
public:
    explicit TypedAssetRef(String short_name)
        : AssetRef(move(short_name))
    {
    }
    virtual ~TypedAssetRef() override = default;

    virtual AssetType type() const override { return T::asset_type(); }

    T& get() const
    {
        return dynamic_cast<T&>(asset());
    }
};

class GenericAssetRef final : public AssetRef {
public:
    GenericAssetRef(String short_name, AssetType type)
        : AssetRef(move(short_name))
        , m_type(type)
    {
    }
    virtual ~GenericAssetRef() override = default;

    virtual AssetType type() const override { return m_type; }

private:
    AssetType m_type;
};

}

using Assets::AssetRef;
using Assets::GenericAssetRef;
using Assets::TypedAssetRef;
