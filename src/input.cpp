#pragma once



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

inline bool keyIsPressed(InputState *input, KeyboardShortcut shortcut)
{
	return keyIsPressed(input, shortcut.key, shortcut.modifiers);
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

inline bool keyWasPressed(InputState *input, KeyboardShortcut shortcut)
{
	return keyWasPressed(input, shortcut.key, shortcut.modifiers);
}

inline bool keyJustPressed(InputState *input, SDL_Keycode key, u8 modifiers=0)
{
	return keyIsPressed(input, key, modifiers) && !keyWasPressed(input, key);
}

inline bool keyJustPressed(InputState *input, KeyboardShortcut shortcut)
{
	return keyJustPressed(input, shortcut.key, shortcut.modifiers);
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
				copyString(event.text.text, strlen(event.text.text), &inputState->textEntered);
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

KeyboardShortcut parseKeyboardShortcut(String input)
{
	KeyboardShortcut result = {};

	String keyName, remainder;
	keyName = nextToken(input, &remainder, '+');

	while (keyName.length > 0)
	{
		// 
		// MODIFIERS
		// 
		if (equals(keyName, "Alt"))
		{
			result.modifiers |= KeyMod_Alt;
		}
		else if (equals(keyName, "Ctrl"))
		{
			result.modifiers |= KeyMod_Ctrl;
		}
		else if (equals(keyName, "Shift"))
		{
			result.modifiers |= KeyMod_Shift;
		}
		else if (equals(keyName, "Super"))
		{
			result.modifiers |= KeyMod_Super;
		}
		// 
		// KEYS
		// 
		else if (equals(keyName, "F1"))
		{
			result.key = SDLK_F1;
		}
		else if (equals(keyName, "F2"))
		{
			result.key = SDLK_F2;
		}
		else if (equals(keyName, "F3"))
		{
			result.key = SDLK_F3;
		}
		else if (equals(keyName, "F4"))
		{
			result.key = SDLK_F4;
		}
		else if (equals(keyName, "F5"))
		{
			result.key = SDLK_F5;
		}
		else if (equals(keyName, "F6"))
		{
			result.key = SDLK_F6;
		}
		else if (equals(keyName, "F7"))
		{
			result.key = SDLK_F7;
		}
		else if (equals(keyName, "F8"))
		{
			result.key = SDLK_F8;
		}
		else if (equals(keyName, "F9"))
		{
			result.key = SDLK_F9;
		}
		else if (equals(keyName, "F10"))
		{
			result.key = SDLK_F10;
		}
		else if (equals(keyName, "F11"))
		{
			result.key = SDLK_F11;
		}
		else if (equals(keyName, "F12"))
		{
			result.key = SDLK_F12;
		}
		// 
		// DUNNO!
		// 
		else
		{
			// Error!
			result.key = SDLK_UNKNOWN;
			break;
		}

		keyName = nextToken(remainder, &remainder, '+');
	}

	return result;
}