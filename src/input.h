#pragma once

const int MOUSE_BUTTON_COUNT = SDL_BUTTON_X2;
const int KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;

struct InputState
{
	// Mouse
	Coord mousePos;
	bool mouseDown[MOUSE_BUTTON_COUNT];
	bool mouseWasDown[MOUSE_BUTTON_COUNT];
	Coord clickStartPosition[MOUSE_BUTTON_COUNT];
	int32 wheelX, wheelY;

	// Keyboard
	bool keyWasDown[KEYBOARD_KEY_COUNT];
	bool keyDown[KEYBOARD_KEY_COUNT];
	char textEntered[SDL_TEXTINPUTEVENT_TEXT_SIZE];
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
inline bool keyJustPressed(InputState *input, SDL_Keycode key) {
	return input->keyDown[key] && !input->keyWasDown[key];
}