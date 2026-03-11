/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Debug/Console.h>
#include <Util/Array.h>
#include <Util/Span.h>

class Keymap final : public Asset {
    ASSET_SUBCLASS_METHODS(Keymap);

public:
    Keymap() = default;
    static ErrorOr<NonnullOwnPtr<Keymap>> load(AssetMetadata&, Blob data);
    virtual ~Keymap() override = default;

    ReadonlySpan<CommandShortcut> shortcuts() const { return m_shortcuts; }

    void unload(AssetMetadata& metadata) override;

private:
    explicit Keymap(Blob data, Array<CommandShortcut>);

    Blob m_data {};
    Array<CommandShortcut> m_shortcuts {};
};
