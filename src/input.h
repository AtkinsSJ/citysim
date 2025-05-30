/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

int const KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;

enum MouseButton {
    // NB: There is no button for 0, so any arrays of foo[MouseButtonCount] never use the first element.
    // I figure this is more straightforward than having to remember to subtract 1 from the button index,
    // or using different values than SDL does. "Wasting" one element is no big deal.
    // - Sam, 27/06/2019
    MouseButton_Left = SDL_BUTTON_LEFT,
    MouseButton_Middle = SDL_BUTTON_MIDDLE,
    MouseButton_Right = SDL_BUTTON_RIGHT,
    MouseButton_X1 = SDL_BUTTON_X1,
    MouseButton_X2 = SDL_BUTTON_X2,
    MouseButtonCount
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
    bool mouseDown[MouseButtonCount];
    bool mouseWasDown[MouseButtonCount];
    V2 clickStartPosNormalised[MouseButtonCount]; // In normalised (-1 to 1) coordinates
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
V2 getClickStartPos(MouseButton mouseButton, struct Camera* camera);

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
