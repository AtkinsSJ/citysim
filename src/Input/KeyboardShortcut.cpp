/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "KeyboardShortcut.h"
#include <Debug/Debug.h>
#include <Util/Log.h>
#include <Util/TokenReader.h>

/**
 * NB: Right now, we only support a very small number of shortcut key types.
 * Add fancier stuff as needed.
 *
 * Basic format of a shortcut is, eg:
 * Ctrl+Alt+Shift+Super+Home
 *
 * If any key in the sequence is unrecognised, we return SDLK_UNKNOWN for the `key` field.
 *
 * Only one "key", plus any combination of modifiers, is supported.
 * Note that this means you can't bind something to just pressing a modifier key!
 */

KeyboardShortcut::KeyboardShortcut(SDL_Keycode key, Flags<ModifierKey>&& modifiers)
    : m_key(key)
    , m_modifiers(move(modifiers))
{
}

Optional<KeyboardShortcut> KeyboardShortcut::from_string(StringView shortcut_string)
{
    DEBUG_FUNCTION();

    TokenReader tokens { shortcut_string };

    auto key_name = tokens.next_token('+');
    Flags<ModifierKey> modifiers;
    while (key_name.has_value()) {
        //
        // MODIFIERS
        //
        if (key_name == "Alt"_s) {
            modifiers.add(ModifierKey::Alt);
        } else if (key_name == "Ctrl"_s) {
            modifiers.add(ModifierKey::Ctrl);
        } else if (key_name == "Shift"_s) {
            modifiers.add(ModifierKey::Shift);
        } else if (key_name == "Super"_s) {
            modifiers.add(ModifierKey::Super);
        } else {
            // FIXME: Make HashTable compatible with StringViews.
            auto key_string = key_name.value().deprecated_to_string();
            if (auto found_key = input_state().keyNames.find_value(key_string); found_key.has_value())
                return KeyboardShortcut { found_key.release_value(), move(modifiers) };

            // Error!
            logWarn("Unrecognised key name '{0}' in shortcut string '{1}'"_s, { key_name.value(), shortcut_string });
            return {};
        }

        key_name = tokens.next_token('+');
    }
    return {};
}

bool KeyboardShortcut::was_just_pressed() const
{
    return keyJustPressed(m_key, m_modifiers, true);
}
