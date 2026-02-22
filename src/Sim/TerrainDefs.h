/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Util/Array.h>
#include <Util/Blob.h>
#include <Util/String.h>

class TerrainDefs final : public Asset {
    ASSET_SUBCLASS_METHODS(TerrainDefs);

public:
    static ErrorOr<NonnullOwnPtr<TerrainDefs>> load(AssetMetadata&, Blob data);
    TerrainDefs() = default;
    virtual ~TerrainDefs() override = default;

    ReadonlySpan<String> terrain_ids() const { return m_terrain_ids; }

    virtual void unload(AssetMetadata& metadata) override;

private:
    TerrainDefs(Blob data, Array<String> terrain_ids);
    Blob m_data;
    Array<String> m_terrain_ids;
};
