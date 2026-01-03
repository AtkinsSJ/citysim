/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetType.h>
#include <Assets/Forward.h>
#include <Util/String.h>

class AssetRef {
public:
    explicit AssetRef(AssetType type, String short_name)
        : m_type(type)
        , m_name(short_name)
    {
    }

    Asset& get() const;

    AssetType type() const { return m_type; }
    String const& name() const { return m_name; }

private:
    AssetType m_type;
    String m_name;

    mutable Asset* m_pointer { nullptr };
    mutable u32 m_asset_generation { 0 };
};
