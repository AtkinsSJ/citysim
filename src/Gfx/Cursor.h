/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <SDL2/SDL_mouse.h>
#include <Util/Forward.h>

class Cursor final : public Asset {
public:
    static Cursor& get(StringView name);

    static ErrorOr<NonnullOwnPtr<Asset>> load_defs(AssetMetadata&, Blob);
    Cursor();
    virtual ~Cursor() override = default;

    void set_as_cursor() const;

    virtual void unload(AssetMetadata& metadata) override;

private:
    explicit Cursor(SDL_Cursor*);

    SDL_Cursor* m_sdl_cursor;
};
