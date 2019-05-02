#pragma once

const int MOUSE_BUTTON_COUNT = SDL_BUTTON_X2;
const int KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;

enum ModifierKey
{
	KeyMod_Alt   = 1 << 0,
	KeyMod_Ctrl  = 1 << 1,
	KeyMod_Shift = 1 << 2,
	KeyMod_Super = 1 << 3,
};

struct KeyboardShortcut
{
	SDL_Keycode key;
	u8 modifiers;
};

struct InputState
{
	// Mouse
	V2I mousePosRaw;
	V2 mousePosNormalised; // In normalised (-1 to 1) coordinates
	bool mouseDown[MOUSE_BUTTON_COUNT];
	bool mouseWasDown[MOUSE_BUTTON_COUNT];
	V2 clickStartPosNormalised[MOUSE_BUTTON_COUNT]; // In normalised (-1 to 1) coordinates
	s32 wheelX, wheelY;

	// Keyboard
	bool _keyWasDown[KEYBOARD_KEY_COUNT];
	bool _keyDown[KEYBOARD_KEY_COUNT];
	bool hasUnhandledTextEntered; // Has anyone requested the _textEntered?
	String textEntered;
	char _textEntered[SDL_TEXTINPUTEVENT_TEXT_SIZE];

	// Extra
	bool receivedQuitSignal;
	bool wasWindowResized;
	union {
		V2I windowSize;
		struct{s32 windowWidth, windowHeight;};
	};
};