/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Input/Input.h>
#include <SDL2/SDL_keycode.h>
#include <Util/Flags.h>
#include <Util/Optional.h>

struct KeyboardShortcut {
public:
    static Optional<KeyboardShortcut> from_string(StringView);

    bool was_just_pressed() const;

private:
    KeyboardShortcut(SDL_Keycode, Flags<ModifierKey>&&);
    SDL_Keycode m_key;
    Flags<ModifierKey> m_modifiers;
};
