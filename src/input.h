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

	// Extra
	bool receivedQuitSignal;
	bool wasWindowResized;
	int32 newWindowWidth, newWindowHeight;
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


void updateInput(InputState *inputState)
{
	// Clear mouse state
	inputState->wheelX = 0;
	inputState->wheelY = 0;

	for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
		inputState->mouseWasDown[i] = inputState->mouseDown[i];
	}

	for (int i=0; i < KEYBOARD_KEY_COUNT; i++) {
		inputState->keyWasDown[i] = inputState->keyDown[i];
	}

	inputState->textEntered[0] = 0;

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
						inputState->newWindowWidth = event.window.data1;
						inputState->newWindowHeight = event.window.data2;
					} break;
				}
			} break;

			// MOUSE EVENTS
			// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
			case SDL_MOUSEMOTION: {
				inputState->mousePos.x = event.motion.x;
				inputState->mousePos.y = event.motion.y;
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
				inputState->keyDown[event.key.keysym.scancode] = true;
			} break;
			case SDL_KEYUP: {
				inputState->keyDown[event.key.keysym.scancode] = false;
			} break;
			case SDL_TEXTINPUT: {
				strncpy(inputState->textEntered, event.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
			} break;
		}
	}

	for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
		if (mouseButtonJustPressed(inputState, i)) {
			// Store the initial click position
			inputState->clickStartPosition[mouseButtonIndex(i)] = inputState->mousePos;
		}
	}
}