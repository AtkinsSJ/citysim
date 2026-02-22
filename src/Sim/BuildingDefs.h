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

class BuildingDefs final : public Asset {
    ASSET_SUBCLASS_METHODS(BuildingDefs);

public:
    static ErrorOr<NonnullOwnPtr<BuildingDefs>> load(AssetMetadata&, Blob data);
    BuildingDefs() = default;
    virtual ~BuildingDefs() override = default;

    ReadonlySpan<String> building_ids() const { return m_building_ids; }

    virtual void unload(AssetMetadata& metadata) override;

private:
    BuildingDefs(Blob data, Array<String> building_ids);
    Blob m_data;
    Array<String> m_building_ids;
};
