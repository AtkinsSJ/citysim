/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "input.h"
#include "AppState.h"
#include <Gfx/Renderer.h>
#include <SDL2/SDL_clipboard.h>

static InputState s_input_state;

u32 keycodeToIndex(u32 key)
{
    return key & ~SDLK_SCANCODE_MASK;
}

void init_input_state()
{
    s_input_state = {};

    MemoryArena* systemArena = &AppState::the().systemArena;

    s_input_state.textEntered = makeString(&s_input_state._textEntered[0], SDL_TEXTINPUTEVENT_TEXT_SIZE);
    s_input_state.textEnteredLength = 0;

    // Key names
    initHashTable(&s_input_state.keyNames, 0.75f, SDL_NUM_SCANCODES);

    // Letters
    for (char c = 'A'; c <= 'Z'; c++) {
        String key = pushString(systemArena, 1);
        key[0] = c;
        s_input_state.keyNames.put(key, (SDL_Keycode)(SDLK_a + (c - 'A')));
    }

    // Numbers
    for (char i = 0; i <= 9; i++) {
        String key = pushString(systemArena, 1);
        key[0] = '0' + i;
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
    DEBUG_FUNCTION_T(DCDT_Input);

    // Clear mouse state
    s_input_state.wheelX = 0;
    s_input_state.wheelY = 0;

    // Fill-in "was down" info.
    copyMemory(s_input_state.mouseDown, s_input_state.mouseWasDown, MouseButtonCount);
    // NB: We don't want to simply ping-pong buffers here, because keyDown records whether it's down NOW,
    // and NOT whether it changed! I wasted some time doing ping-pong and being confused about it. /fp
    // - Sam, 10/07/2019
    copyMemory(s_input_state._keyDown, s_input_state._keyWasDown, KEYBOARD_KEY_COUNT);

    s_input_state.hasUnhandledTextEntered = false;
    s_input_state.textEnteredLength = 0;

    s_input_state.receivedQuitSignal = false;

    {
        DEBUG_BLOCK_T("updateInput: Pump", DCDT_Input);
        SDL_PumpEvents();
    }
    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
        DEBUG_BLOCK_T("updateInput: Event", DCDT_Input);

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

    for (s32 i = 1; i < MouseButtonCount; i++) {
        MouseButton button = MouseButton(i);
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
    bool result = false;

    switch (modifier) {
    case KeyMod_Alt:
        result = (s_input_state._keyDown[keycodeToIndex(SDLK_LALT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RALT)]);
        break;
    case KeyMod_Ctrl:
        result = (s_input_state._keyDown[keycodeToIndex(SDLK_LCTRL)] || s_input_state._keyDown[keycodeToIndex(SDLK_RCTRL)]);
        break;
    case KeyMod_Shift:
        result = (s_input_state._keyDown[keycodeToIndex(SDLK_LSHIFT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RSHIFT)]);
        break;
    case KeyMod_Super:
        result = (s_input_state._keyDown[keycodeToIndex(SDLK_LGUI)] || s_input_state._keyDown[keycodeToIndex(SDLK_RGUI)]);
        break;
    }

    return result;
}

u8 getPressedModifierKeys()
{
    u8 result = 0;

    if (modifierKeyIsPressed(KeyMod_Alt))
        result |= KeyMod_Alt;
    if (modifierKeyIsPressed(KeyMod_Ctrl))
        result |= KeyMod_Ctrl;
    if (modifierKeyIsPressed(KeyMod_Shift))
        result |= KeyMod_Shift;
    if (modifierKeyIsPressed(KeyMod_Super))
        result |= KeyMod_Super;

    return result;
}

bool modifierKeysArePressed(u8 modifiers)
{
    bool result = true;

    if (modifiers) {
        if (modifiers & KeyMod_Alt) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LALT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RALT)]);
        }
        if (modifiers & KeyMod_Ctrl) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LCTRL)] || s_input_state._keyDown[keycodeToIndex(SDLK_RCTRL)]);
        }
        if (modifiers & KeyMod_Shift) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LSHIFT)] || s_input_state._keyDown[keycodeToIndex(SDLK_RSHIFT)]);
        }
        if (modifiers & KeyMod_Super) {
            result = result && (s_input_state._keyDown[keycodeToIndex(SDLK_LGUI)] || s_input_state._keyDown[keycodeToIndex(SDLK_RGUI)]);
        }
    }

    return result;
}

bool keyIsPressed(SDL_Keycode key, u8 modifiers)
{
    s32 keycode = keycodeToIndex(key);

    bool result = s_input_state._keyDown[keycode];

    if (modifiers) {
        result = result && modifierKeysArePressed(modifiers);
    }

    return result;
}

bool keyWasPressed(SDL_Keycode key, u8 modifiers)
{
    s32 keycode = keycodeToIndex(key);

    bool result = s_input_state._keyWasDown[keycode];

    if (modifiers) {
        if (modifiers & KeyMod_Alt) {
            result = result && (keyWasPressed(SDLK_LALT) || keyWasPressed(SDLK_RALT));
        }
        if (modifiers & KeyMod_Ctrl) {
            result = result && (keyWasPressed(SDLK_LCTRL) || keyWasPressed(SDLK_RCTRL));
        }
        if (modifiers & KeyMod_Shift) {
            result = result && (keyWasPressed(SDLK_LSHIFT) || keyWasPressed(SDLK_RSHIFT));
        }
        if (modifiers & KeyMod_Super) {
            result = result && (keyWasPressed(SDLK_LGUI) || keyWasPressed(SDLK_RGUI));
        }
    }

    return result;
}

bool keyJustPressed(SDL_Keycode key, u8 modifiers, bool ignoreRepeats)
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

bool wasShortcutJustPressed(KeyboardShortcut shortcut)
{
    return keyJustPressed(shortcut.key, shortcut.modifiers, true);
}

bool wasTextEntered()
{
    return s_input_state.hasUnhandledTextEntered;
}

String getEnteredText()
{
    s_input_state.hasUnhandledTextEntered = false;
    return makeString(s_input_state.textEntered.chars, s_input_state.textEnteredLength);
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
    keyName = nextToken(shortcutString, &remainder, '+');

    while (!isEmpty(keyName)) {
        //
        // MODIFIERS
        //
        if (equals(keyName, "Alt"_s)) {
            result.modifiers |= KeyMod_Alt;
        } else if (equals(keyName, "Ctrl"_s)) {
            result.modifiers |= KeyMod_Ctrl;
        } else if (equals(keyName, "Shift"_s)) {
            result.modifiers |= KeyMod_Shift;
        } else if (equals(keyName, "Super"_s)) {
            result.modifiers |= KeyMod_Super;
        } else {
            Maybe<SDL_Keycode> found = s_input_state.keyNames.findValue(keyName);
            if (found.isValid) {
                result.key = found.value;
            } else {
                // Error!
                logWarn("Unrecognised key name '{0}' in shortcut string '{1}'"_s, { keyName, shortcutString });
                result.key = SDLK_UNKNOWN;
            }
            break;
        }

        keyName = nextToken(remainder, &remainder, '+');
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
