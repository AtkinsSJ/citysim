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

/**
 * MOUSE INPUT
 */
inline u8 mouseButtonIndex(u8 sdlMouseButton) {
	return sdlMouseButton - 1;
}
inline bool mouseButtonJustPressed(InputState *input, u8 mouseButton) {
	u8 buttonIndex = mouseButtonIndex(mouseButton);
	return input->mouseDown[buttonIndex] && !input->mouseWasDown[buttonIndex];
}
inline bool mouseButtonJustReleased(InputState *input, u8 mouseButton) {
	u8 buttonIndex = mouseButtonIndex(mouseButton);
	return !input->mouseDown[buttonIndex] && input->mouseWasDown[buttonIndex];
}
inline bool mouseButtonPressed(InputState *input, u8 mouseButton) {
	return input->mouseDown[mouseButtonIndex(mouseButton)];
}

inline V2 getClickStartPos(InputState *input, u8 mouseButton, struct Camera *camera)
{
	u8 buttonIndex = mouseButtonIndex(mouseButton);
	return unproject(camera, input->clickStartPosNormalised[buttonIndex]);
}

/**
 * KEYBOARD INPUT
 */
#define keycodeToIndex(key) ((key) & ~SDLK_SCANCODE_MASK)

inline bool modifierKeyIsPressed(InputState *input, ModifierKey modifier)
{
	bool result = false;

	switch (modifier)
	{
	case KeyMod_Alt:
		result = (input->_keyDown[keycodeToIndex(SDLK_LALT)] || input->_keyDown[keycodeToIndex(SDLK_RALT)]);
		break;
	case KeyMod_Ctrl:
		result = (input->_keyDown[keycodeToIndex(SDLK_LCTRL)] || input->_keyDown[keycodeToIndex(SDLK_RCTRL)]);
		break;
	case KeyMod_Shift:
		result = (input->_keyDown[keycodeToIndex(SDLK_LSHIFT)] || input->_keyDown[keycodeToIndex(SDLK_RSHIFT)]);
		break;
	case KeyMod_Super:
		result = (input->_keyDown[keycodeToIndex(SDLK_LGUI)] || input->_keyDown[keycodeToIndex(SDLK_RGUI)]);
		break;
	}

	return result;
}

inline bool modifierKeysArePressed(InputState *input, u8 modifiers)
{
	bool result = true;

	if (modifiers)
	{
		if (modifiers & KeyMod_Alt)
		{
			result = result && (input->_keyDown[keycodeToIndex(SDLK_LALT)] || input->_keyDown[keycodeToIndex(SDLK_RALT)]);
		}
		if (modifiers & KeyMod_Ctrl)
		{
			result = result && (input->_keyDown[keycodeToIndex(SDLK_LCTRL)] || input->_keyDown[keycodeToIndex(SDLK_RCTRL)]);
		}
		if (modifiers & KeyMod_Shift)
		{
			result = result && (input->_keyDown[keycodeToIndex(SDLK_LSHIFT)] || input->_keyDown[keycodeToIndex(SDLK_RSHIFT)]);
		}
		if (modifiers & KeyMod_Super)
		{
			result = result && (input->_keyDown[keycodeToIndex(SDLK_LGUI)] || input->_keyDown[keycodeToIndex(SDLK_RGUI)]);
		}
	}

	return result;
}

inline bool keyIsPressed(InputState *input, SDL_Keycode key, u8 modifiers=0)
{
	s32 keycode = keycodeToIndex(key);

	bool result = input->_keyDown[keycode];

	if (modifiers)
	{
		result = result && modifierKeysArePressed(input, modifiers);
	}

	return result;
}

inline bool keyWasPressed(InputState *input, SDL_Keycode key, u8 modifiers=0)
{
	s32 keycode = keycodeToIndex(key);

	bool result = input->_keyWasDown[keycode];

	if (modifiers)
	{
		if (modifiers & KeyMod_Alt)
		{
			result = result && (keyWasPressed(input, SDLK_LALT) || keyWasPressed(input, SDLK_RALT));
		}
		if (modifiers & KeyMod_Ctrl)
		{
			result = result && (keyWasPressed(input, SDLK_LCTRL) || keyWasPressed(input, SDLK_RCTRL));
		}
		if (modifiers & KeyMod_Shift)
		{
			result = result && (keyWasPressed(input, SDLK_LSHIFT) || keyWasPressed(input, SDLK_RSHIFT));
		}
		if (modifiers & KeyMod_Super)
		{
			result = result && (keyWasPressed(input, SDLK_LGUI) || keyWasPressed(input, SDLK_RGUI));
		}
	}

	return result;
}

inline bool keyJustPressed(InputState *input, SDL_Keycode key, u8 modifiers=0)
{
	return keyIsPressed(input, key, modifiers) && !keyWasPressed(input, key);
}

inline bool wasTextEntered(InputState *input)
{
	return input->hasUnhandledTextEntered;
}

inline String getEnteredText(InputState *input)
{
	input->hasUnhandledTextEntered = false;
	return input->textEntered;
}

inline String getClipboardText()
{
	String result = {};

	if (SDL_HasClipboardText())
	{
		char *clipboard = SDL_GetClipboardText();

		if (clipboard)
		{
			result = pushString(globalFrameTempArena, clipboard);
			SDL_free(clipboard);
		}
	}
	
	return result;
}

void initInput(InputState *inputState)
{
	*inputState = {};
	inputState->textEntered = makeString(&inputState->_textEntered[0], SDL_TEXTINPUTEVENT_TEXT_SIZE);
}

void updateInput(InputState *inputState)
{
	DEBUG_FUNCTION();
	
	// Clear mouse state
	inputState->wheelX = 0;
	inputState->wheelY = 0;

	for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
		inputState->mouseWasDown[i] = inputState->mouseDown[i];
	}

	for (int i=0; i < KEYBOARD_KEY_COUNT; i++) {
		inputState->_keyWasDown[i] = inputState->_keyDown[i];
	}

	inputState->hasUnhandledTextEntered = false;
	inputState->textEntered.length = 0;

	inputState->receivedQuitSignal = false;
	inputState->wasWindowResized = false;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			// WINDOW EVENTS
			case SDL_QUIT: {
				inputState->receivedQuitSignal = true;
			} break;
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED: {
						inputState->wasWindowResized = true;
						inputState->windowWidth = event.window.data1;
						inputState->windowHeight = event.window.data2;
					} break;
				}
			} break;

			// MOUSE EVENTS
			// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
			case SDL_MOUSEMOTION: {
				// Mouse pos in screen coordinates
				inputState->mousePosRaw.x = event.motion.x;
				inputState->mousePosRaw.y = event.motion.y;
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				u8 buttonIndex = event.button.button - 1;
				inputState->mouseDown[buttonIndex] = true;
			} break;
			case SDL_MOUSEBUTTONUP: {
				u8 buttonIndex = event.button.button - 1;
				inputState->mouseDown[buttonIndex] = false;
			} break;
			case SDL_MOUSEWHEEL: {
				if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					inputState->wheelX = -event.wheel.x;
					inputState->wheelY = -event.wheel.y;
				} else {
					inputState->wheelX = event.wheel.x;
					inputState->wheelY = event.wheel.y;
				}
			} break;

			// KEYBOARD EVENTS
			case SDL_KEYDOWN: {
				s32 keycode = keycodeToIndex(event.key.keysym.sym);
				inputState->_keyDown[keycode] = true;
				if (event.key.repeat)
				{
					// This is a hack! Well, our whole concept of input handling is weird, so not really.
					// We pretend that we had released the key so that we notice the repeat.
					inputState->_keyWasDown[keycode] = false;
				}
			} break;
			case SDL_KEYUP: {
				s32 keycode = keycodeToIndex(event.key.keysym.sym);
				inputState->_keyDown[keycode] = false;
			} break;
			case SDL_TEXTINPUT: {
				inputState->hasUnhandledTextEntered = true;
				copyChars(event.text.text, &inputState->textEntered, strlen(event.text.text));
			} break;
		}
	}

	inputState->mousePosNormalised.x = ((inputState->mousePosRaw.x * 2.0f) / inputState->windowWidth) - 1.0f;
	inputState->mousePosNormalised.y = ((inputState->mousePosRaw.y * -2.0f) + inputState->windowHeight) / inputState->windowHeight;

	for (u8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
		if (mouseButtonJustPressed(inputState, i)) {
			// Store the initial click position
			inputState->clickStartPosNormalised[mouseButtonIndex(i)] = inputState->mousePosNormalised;
		}
	}
}