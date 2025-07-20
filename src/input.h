/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Forward.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>
#include <Util/Basic.h>
#include <Util/EnumMap.h>
#include <Util/HashTable.h>
#include <Util/String.h>
#include <Util/Vector.h>

int const KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;

enum class MouseButton : u8 {
    // NB: There is no button for 0, so any arrays of foo[MouseButton::COUNT] never use the first element.
    // I figure this is more straightforward than having to remember to subtract 1 from the button index,
    // or using different values than SDL does. "Wasting" one element is no big deal.
    // - Sam, 27/06/2019
    Left = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right = SDL_BUTTON_RIGHT,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2,
    COUNT
};

enum ModifierKey {
    KeyMod_Alt = 1 << 0,
    KeyMod_Ctrl = 1 << 1,
    KeyMod_Shift = 1 << 2,
    KeyMod_Super = 1 << 3,
};

struct KeyboardShortcut {
    SDL_Keycode key;
    u8 modifiers;
};

struct InputState {
    // Mouse
    V2I mousePosRaw;
    V2 mousePosNormalised; // In normalised (-1 to 1) coordinates
    EnumMap<MouseButton, bool> mouseDown;
    EnumMap<MouseButton, bool> mouseWasDown;
    EnumMap<MouseButton, V2> clickStartPosNormalised; // In normalised (-1 to 1) coordinates
    s32 wheelX, wheelY;

    // Keyboard
    bool _keyWasDown[KEYBOARD_KEY_COUNT];
    bool _keyDown[KEYBOARD_KEY_COUNT];
    bool _keyDownIsRepeat[KEYBOARD_KEY_COUNT];

    bool hasUnhandledTextEntered; // Has anyone requested the _textEntered?
    String textEntered;
    s32 textEnteredLength;
    char _textEntered[SDL_TEXTINPUTEVENT_TEXT_SIZE];

    // Extra
    bool receivedQuitSignal;

    HashTable<SDL_Keycode> keyNames;

    void* capturedInputTarget; // eg, a TextInput that's currently consuming all key presses
};

//
// PUBLIC
//
void init_input_state();
InputState& input_state();
void updateInput();

bool mouseButtonJustPressed(MouseButton mouseButton);
bool mouseButtonJustReleased(MouseButton mouseButton);
bool mouseButtonPressed(MouseButton mouseButton);
V2 getClickStartPos(MouseButton mouseButton, Camera* camera);

bool modifierKeyIsPressed(ModifierKey modifier);
bool keyIsPressed(SDL_Keycode key, u8 modifiers = 0);
bool keyWasPressed(SDL_Keycode key, u8 modifiers = 0);
bool keyJustPressed(SDL_Keycode key, u8 modifiers = 0, bool ignoreRepeats = false);

KeyboardShortcut parseKeyboardShortcut(String shortcutString);
bool wasShortcutJustPressed(KeyboardShortcut shortcut);

bool wasTextEntered();
String getEnteredText();

String getClipboardText();

// eg, a TextInput that's currently consuming all key presses
void captureInput(void* target);
void releaseInput(void* target);
bool hasCapturedInput(void* target);
bool isInputCaptured();

//
// INTERNAL
//

u32 keycodeToIndex(u32 key);
u8 getPressedModifierKeys();
bool modifierKeysArePressed(u8 modifiers);
