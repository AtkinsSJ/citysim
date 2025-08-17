/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "input.h"
#include "AppState.h"
#include <Gfx/Renderer.h>
#include <SDL2/SDL_clipboard.h>

static InputState s_input_state {};

u32 keycodeToIndex(u32 key)
{
    return key & ~SDLK_SCANCODE_MASK;
}

void init_input_state()
{
    MemoryArena* systemArena = &AppState::the().systemArena;

    s_input_state.textEntered = String { &s_input_state._textEntered[0], SDL_TEXTINPUTEVENT_TEXT_SIZE };
    s_input_state.textEnteredLength = 0;

    // Letters
    for (char c = 'A'; c <= 'Z'; c++) {
        String key = pushString(systemArena, 1);
        key.chars[0] = c;
        s_input_state.keyNames.put(key, (SDL_Keycode)(SDLK_a + (c - 'A')));
    }

    // Numbers
    for (char i = 0; i <= 9; i++) {
        String key = pushString(systemArena, 1);
        key.chars[0] = '0' + i;
        s_input_state.keyNames.put(key, (SDL_Keycode)(SDLK_0 + i));
    }

    // F keys
    for (char i = 0; i <= 12; i++) {
        String key = pushString(systemArena, myprintf("F{0}"_s, { formatInt(i + 1) }));
        s_input_state.keyNames.put(key, (SDL_Keycode)(SDLK_F1 + i));
    }

    // Misc
    s_input_state.keyNames.put(pushString(systemArena, "Home"), (SDL_Keycode)SDLK_HOME);
}

InputState& input_state()
{
    return s_input_state;
}

void updateInput()
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Input);

    // Clear mouse state
    s_input_state.wheelX = 0;
    s_input_state.wheelY = 0;

    // Fill-in "was down" info.
    s_input_state.mouseWasDown = s_input_state.mouseDown;
    // NB: We don't want to simply ping-pong buffers here, because keyDown records whether it's down NOW,
    // and NOT whether it changed! I wasted some time doing ping-pong and being confused about it. /fp
    // - Sam, 10/07/2019
    copyMemory(s_input_state._keyDown, s_input_state._keyWasDown, KEYBOARD_KEY_COUNT);

    s_input_state.hasUnhandledTextEntered = false;
    s_input_state.textEnteredLength = 0;

    s_input_state.receivedQuitSignal = false;

    {
        DEBUG_BLOCK_T("updateInput: Pump", DebugCodeDataTag::Input);
        SDL_PumpEvents();
    }
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
        DEBUG_BLOCK_T("updateInput: Event", DebugCodeDataTag::Input);

        switch (event.type) {
        // WINDOW EVENTS
        case SDL_QUIT: {
            s_input_state.receivedQuitSignal = true;
        } break;
        case SDL_WINDOWEVENT: {
            the_renderer().handle_window_event(event.window);
        } break;

        // MOUSE EVENTS
        // NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
        case SDL_MOUSEMOTION: {
            // Mouse pos in screen coordinates
            s_input_state.mousePosRaw.x = event.motion.x;
            s_input_state.mousePosRaw.y = event.motion.y;
        } break;
        case SDL_MOUSEBUTTONDOWN: {
            u8 buttonIndex = event.button.button;
            s_input_state.mouseDown[buttonIndex] = true;
        } break;
        case SDL_MOUSEBUTTONUP: {
            u8 buttonIndex = event.button.button;
            s_input_state.mouseDown[buttonIndex] = false;
        } break;
        case SDL_MOUSEWHEEL: {
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                s_input_state.wheelX = -event.wheel.x;
                s_input_state.wheelY = -event.wheel.y;
            } else {
                s_input_state.wheelX = event.wheel.x;
                s_input_state.wheelY = event.wheel.y;
            }
        } break;

        // KEYBOARD EVENTS
        case SDL_KEYDOWN: {
            s32 keycode = keycodeToIndex(event.key.keysym.sym);
            s_input_state._keyDown[keycode] = true;
            s_input_state._keyDownIsRepeat[keycode] = (event.key.repeat > 0);
            if (event.key.repeat) {
                // Slight hack: We detect a key press if _keyDown is true and _keyWasDown is false.
                // This hides repeats!
                // So, we falsely say _keyWasDown is false, so the repeat gets noticed.
                // You can distinguish between repeats and normal presses with _keyDownIsRepeat.
                // keyJustPressed() also has a parameter for ignoring repeats.
                s_input_state._keyWasDown[keycode] = false;
            }
        } break;
        case SDL_KEYUP: {
            s32 keycode = keycodeToIndex(event.key.keysym.sym);
            s_input_state._keyDown[keycode] = false;
            s_input_state._keyDownIsRepeat[keycode] = false;
        } break;
        case SDL_TEXTINPUT: {
            s_input_state.hasUnhandledTextEntered = true;
            s_input_state.textEnteredLength = truncate32(strlen(event.text.text));
            copyString(event.text.text, s_input_state.textEnteredLength, &s_input_state.textEntered);
        } break;
        }
    }

    auto& renderer = the_renderer();
    auto const window_size = renderer.window_size();
    s_input_state.mousePosNormalised.x = ((s_input_state.mousePosRaw.x * 2.0f) / window_size.x) - 1.0f;
    s_input_state.mousePosNormalised.y = ((s_input_state.mousePosRaw.y * -2.0f) + window_size.y) / window_size.y;

    for (auto button : enum_values<MouseButton>()) {
        if (mouseButtonJustPressed(button)) {
            // Store the initial click position
            s_input_state.clickStartPosNormalised[button] = s_input_state.mousePosNormalised;
        }
    }
}

/**
 * MOUSE INPUT
 */
bool mouseButtonJustPressed(MouseButton mouseButton)
{
    return s_input_state.mouseDown[mouseButton] && !s_input_state.mouseWasDown[mouseButton];
}
bool mouseButtonJustReleased(MouseButton mouseButton)
{
    return !s_input_state.mouseDown[mouseButton] && s_input_state.mouseWasDown[mouseButton];
}
bool mouseButtonPressed(MouseButton mouseButton)
{
    return s_input_state.mouseDown[mouseButton];
}

V2 getClickStartPos(MouseButton mouseButton, Camera* camera)
{
    return camera->unproject(s_input_state.clickStartPosNormalised[mouseButton]);
}

/**
 * KEYBOARD INPUT
 */

bool modifierKeyIsPressed(ModifierKey modifier)
{
    switch (modifier) {
    case ModifierKey::Alt:
        return s_input_state._keyDown[keycodeToIndex(SDLK_LALT)]
            || s_input_state._keyDown[keycodeToIndex(SDLK_RALT)];
    case ModifierKey::Ctrl:
        return s_input_state._keyDown[keycodeToIndex(SDLK_LCTRL)]
            || s_input_state._keyDown[keycodeToIndex(SDLK_RCTRL)];
    case ModifierKey::Shift:
        return s_input_state._keyDown[keycodeToIndex(SDLK_LSHIFT)]
            || s_input_state._keyDown[keycodeToIndex(SDLK_RSHIFT)];
    case ModifierKey::Super:
        return s_input_state._keyDown[keycodeToIndex(SDLK_LGUI)]
            || s_input_state._keyDown[keycodeToIndex(SDLK_RGUI)];
    case ModifierKey::COUNT:
        break;
    }

    VERIFY_NOT_REACHED();
}

Flags<ModifierKey> getPressedModifierKeys()
{
    Flags<ModifierKey> result;

    if (modifierKeyIsPressed(ModifierKey::Alt))
        result.add(ModifierKey::Alt);
    if (modifierKeyIsPressed(ModifierKey::Ctrl))
        result.add(ModifierKey::Ctrl);
    if (modifierKeyIsPressed(ModifierKey::Shift))
        result.add(ModifierKey::Shift);
    if (modifierKeyIsPressed(ModifierKey::Super))
        result.add(ModifierKey::Super);

    return result;
}

bool modifierKeysArePressed(Flags<ModifierKey> modifiers)
{
    bool result = true;

    if (modifiers) {
        if (modifiers.has(ModifierKey::Alt)) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LALT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RALT)]);
        }
        if (modifiers.has(ModifierKey::Ctrl)) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LCTRL)] || s_input_state._keyDown[keycodeToIndex(SDLK_RCTRL)]);
        }
        if (modifiers.has(ModifierKey::Shift)) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LSHIFT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RSHIFT)]);
        }
        if (modifiers.has(ModifierKey::Super)) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LGUI)] || s_input_state._keyDown[keycodeToIndex(SDLK_RGUI)]);
        }
    }

    return result;
}

bool keyIsPressed(SDL_Keycode key, Flags<ModifierKey> modifiers)
{
    s32 keycode = keycodeToIndex(key);

    bool result = s_input_state._keyDown[keycode];

    if (modifiers) {
        result = result && modifierKeysArePressed(modifiers);
    }

    return result;
}

bool keyWasPressed(SDL_Keycode key, Flags<ModifierKey> modifiers)
{
    s32 keycode = keycodeToIndex(key);

    bool result = s_input_state._keyWasDown[keycode];

    if (modifiers) {
        if (modifiers.has(ModifierKey::Alt)) {
            result = result && (keyWasPressed(SDLK_LALT) || keyWasPressed(SDLK_RALT));
        }
        if (modifiers.has(ModifierKey::Ctrl)) {
            result = result && (keyWasPressed(SDLK_LCTRL) || keyWasPressed(SDLK_RCTRL));
        }
        if (modifiers.has(ModifierKey::Shift)) {
            result = result && (keyWasPressed(SDLK_LSHIFT) || keyWasPressed(SDLK_RSHIFT));
        }
        if (modifiers.has(ModifierKey::Super)) {
            result = result && (keyWasPressed(SDLK_LGUI) || keyWasPressed(SDLK_RGUI));
        }
    }

    return result;
}

bool keyJustPressed(SDL_Keycode key, Flags<ModifierKey> modifiers, bool ignoreRepeats)
{
    bool result = keyIsPressed(key, modifiers) && !keyWasPressed(key);

    if (result && ignoreRepeats) {
        s32 keycode = keycodeToIndex(key);

        // To ignore the repeat, we just return the opposite of whether it's a repeat.
        // The result is always true before this line.
        result = !s_input_state._keyDownIsRepeat[keycode];
    }

    return result;
}

bool wasShortcutJustPressed(KeyboardShortcut const& shortcut)
{
    return keyJustPressed(shortcut.key, shortcut.modifiers, true);
}

bool wasTextEntered()
{
    return s_input_state.hasUnhandledTextEntered;
}

StringView getEnteredText()
{
    s_input_state.hasUnhandledTextEntered = false;
    return StringView { s_input_state.textEntered.chars, (size_t)s_input_state.textEnteredLength };
}

String getClipboardText()
{
    String result = {};

    if (SDL_HasClipboardText()) {
        char* clipboard = SDL_GetClipboardText();

        if (clipboard) {
            result = pushString(&temp_arena(), clipboard);
            SDL_free(clipboard);
        }
    }

    return result;
}

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

KeyboardShortcut parseKeyboardShortcut(String shortcutString)
{
    DEBUG_FUNCTION();

    KeyboardShortcut result = {};

    String keyName, remainder;
    keyName = shortcutString.next_token(&remainder, '+');

    while (!keyName.is_empty()) {
        //
        // MODIFIERS
        //
        if (keyName == "Alt"_s) {
            result.modifiers.add(ModifierKey::Alt);
        } else if (keyName == "Ctrl"_s) {
            result.modifiers.add(ModifierKey::Ctrl);
        } else if (keyName == "Shift"_s) {
            result.modifiers.add(ModifierKey::Shift);
        } else if (keyName == "Super"_s) {
            result.modifiers.add(ModifierKey::Super);
        } else {
            if (auto found_key = s_input_state.keyNames.find_value(keyName); found_key.has_value()) {
                result.key = found_key.release_value();
            } else {
                // Error!
                logWarn("Unrecognised key name '{0}' in shortcut string '{1}'"_s, { keyName, shortcutString });
                result.key = SDLK_UNKNOWN;
            }
            break;
        }

        keyName = remainder.next_token(&remainder, '+');
    }

    return result;
}

void captureInput(void* target)
{
    s_input_state.capturedInputTarget = target;
}

void releaseInput(void* target)
{
    if (s_input_state.capturedInputTarget == target) {
        s_input_state.capturedInputTarget = nullptr;
    }
}

bool hasCapturedInput(void* target)
{
    return s_input_state.capturedInputTarget == target;
}

bool isInputCaptured()
{
    return s_input_state.capturedInputTarget != nullptr;
}
