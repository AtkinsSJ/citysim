#pragma once

/**
 * MOUSE INPUT
 */

const int MOUSE_BUTTON_COUNT = SDL_BUTTON_X2;
struct MouseState {
	int32 x,y;
	bool down[MOUSE_BUTTON_COUNT];
	bool wasDown[MOUSE_BUTTON_COUNT];
	Coord clickStartPosition[MOUSE_BUTTON_COUNT];
	int32 wheelX, wheelY;
};
inline uint8 mouseButtonIndex(uint8 sdlMouseButton) {
	return sdlMouseButton - 1;
}
inline bool mouseButtonJustPressed(MouseState *mouseState, uint8 mouseButton) {
	uint8 buttonIndex = mouseButtonIndex(mouseButton);
	return mouseState->down[buttonIndex] && !mouseState->wasDown[buttonIndex];
}
inline bool mouseButtonJustReleased(MouseState *mouseState, uint8 mouseButton) {
	uint8 buttonIndex = mouseButtonIndex(mouseButton);
	return !mouseState->down[buttonIndex] && mouseState->wasDown[buttonIndex];
}
inline bool mouseButtonPressed(MouseState *mouseState, uint8 mouseButton) {
	return mouseState->down[mouseButtonIndex(mouseButton)];
}

/**
 * KEYBOARD INPUT
 */

const int KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;
struct KeyboardState {
	bool down[KEYBOARD_KEY_COUNT];
};