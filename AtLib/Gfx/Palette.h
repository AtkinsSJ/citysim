/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Gfx/Colour.h>
#include <Util/Array.h>

class Palette final : public Asset {
    ASSET_SUBCLASS_METHODS(Palette);

public:
    enum class Type : u8 {
        Fixed,
        Gradient,
    };

    Palette() = default;
    Palette(Type, Array<Colour>);
    static ErrorOr<NonnullOwnPtr<Asset>> load_defs(AssetMetadata&, Blob);

    Type type() const { return m_type; }
    size_t size() const;
    Colour colour_at(size_t index) const;
    Colour first() const;
    Colour last() const;
    ReadonlySpan<Colour> colours() const { return m_colours; }

    virtual void unload(AssetMetadata&) override;

private:
    Type m_type { Type::Fixed };
    Array<Colour> m_colours;
};
