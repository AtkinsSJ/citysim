/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Util/Array.h>
#include <Util/Blob.h>
#include <Util/ErrorOr.h>
#include <Util/String.h>

namespace Assets {

class Texts final : public Asset {
    ASSET_SUBCLASS_METHODS(Texts);

public:
    static ErrorOr<NonnullOwnPtr<Texts>> load(AssetMetadata&, Blob data, bool is_fallback_locale);
    Texts() = default;
    virtual ~Texts() override = default;

    virtual void unload(AssetMetadata& metadata) override;

private:
    Texts(Blob data, Array<String> keys, bool is_fallback_locale);
    Blob m_data;
    Array<String> m_keys;
    bool m_is_fallback_locale;
};

}
