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
	Coord mousePosRaw;
	V2 mousePosNormalised; // In normalised (-1 to 1) coordinates
	bool mouseDown[MOUSE_BUTTON_COUNT];
	bool mouseWasDown[MOUSE_BUTTON_COUNT];
	V2 clickStartPosition[MOUSE_BUTTON_COUNT]; // Normalised
	int32 wheelX, wheelY;

	// Keyboard
	bool _keyWasDown[KEYBOARD_KEY_COUNT];
	bool _keyDown[KEYBOARD_KEY_COUNT];
	bool hasUnhandledTextEntered; // Has anyone requested the _textEntered?
	char _textEntered[SDL_TEXTINPUTEVENT_TEXT_SIZE];

	// Extra
	bool receivedQuitSignal;
	bool wasWindowResized;
	union {
		Coord windowSize;
		struct{int32 windowWidth, windowHeight;};
	};
};

/**
 * MOUSE INPUT
 */
inline uint8 mouseButtonIndex(uint8 sdlMouseButton) {
	return sdlMouseButton - 1;
}
inline bool mouseButtonJustPressed(InputState *input, uint8 mouseButton) {
	uint8 buttonIndex = mouseButtonIndex(mouseButton);
	return input->mouseDown[buttonIndex] && !input->mouseWasDown[buttonIndex];
}
inline bool mouseButtonJustReleased(InputState *input, uint8 mouseButton) {
	uint8 buttonIndex = mouseButtonIndex(mouseButton);
	return !input->mouseDown[buttonIndex] && input->mouseWasDown[buttonIndex];
}
inline bool mouseButtonPressed(InputState *input, uint8 mouseButton) {
	return input->mouseDown[mouseButtonIndex(mouseButton)];
}

/**
 * KEYBOARD INPUT
 */
#define keycodeToIndex(key) ((key) & ~SDLK_SCANCODE_MASK)
inline bool keyIsPressed(InputState *input, SDL_Keycode key, uint8 modifiers=0)
{
	int32 keycode = keycodeToIndex(key);

	bool result = input->_keyDown[keycode];

	if (modifiers)
	{
		if (modifiers & KeyMod_Alt)
		{
			result = result && (keyIsPressed(input, SDLK_LALT) || keyIsPressed(input, SDLK_RALT));
		}
		if (modifiers & KeyMod_Ctrl)
		{
			result = result && (keyIsPressed(input, SDLK_LCTRL) || keyIsPressed(input, SDLK_RCTRL));
		}
		if (modifiers & KeyMod_Shift)
		{
			result = result && (keyIsPressed(input, SDLK_LSHIFT) || keyIsPressed(input, SDLK_RSHIFT));
		}
		if (modifiers & KeyMod_Super)
		{
			result = result && (keyIsPressed(input, SDLK_LGUI) || keyIsPressed(input, SDLK_RGUI));
		}
	}

	return result;
}
inline bool keyWasPressed(InputState *input, SDL_Keycode key, uint8 modifiers=0)
{
	int32 keycode = keycodeToIndex(key);

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
inline bool keyJustPressed(InputState *input, SDL_Keycode key, uint8 modifiers=0)
{
	return keyIsPressed(input, key, modifiers) && !keyWasPressed(input, key);
}

inline bool wasTextEntered(InputState *input)
{
	return input->hasUnhandledTextEntered;
}
inline char *getEnteredText(InputState *input)
{
	input->hasUnhandledTextEntered = false;
	return input->_textEntered;
}

void updateInput(InputState *inputState, int32 windowWidth, int32 windowHeight)
{
	DEBUG_FUNCTION();
	inputState->windowWidth = windowWidth;
	inputState->windowHeight = windowHeight;
	
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
	inputState->_textEntered[0] = 0;

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
				uint8 buttonIndex = event.button.button - 1;
				inputState->mouseDown[buttonIndex] = true;
			} break;
			case SDL_MOUSEBUTTONUP: {
				uint8 buttonIndex = event.button.button - 1;
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
				int32 keycode = keycodeToIndex(event.key.keysym.sym);
				inputState->_keyDown[keycode] = true;
			} break;
			case SDL_KEYUP: {
				int32 keycode = keycodeToIndex(event.key.keysym.sym);
				inputState->_keyDown[keycode] = false;
			} break;
			case SDL_TEXTINPUT: {
				inputState->hasUnhandledTextEntered = true;
				strncpy(inputState->_textEntered, event.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
			} break;
		}
	}

	inputState->mousePosNormalised.x = ((inputState->mousePosRaw.x * 2.0f) / inputState->windowWidth) - 1.0f;
	inputState->mousePosNormalised.y = ((inputState->mousePosRaw.y * -2.0f) + inputState->windowHeight) / inputState->windowHeight;

	for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
		if (mouseButtonJustPressed(inputState, i)) {
			// Store the initial click position
			inputState->clickStartPosition[mouseButtonIndex(i)] = inputState->mousePosNormalised;
		}
	}
}