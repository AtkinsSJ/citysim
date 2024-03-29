#pragma once

inline u32 keycodeToIndex(u32 key)
{
	return key & ~SDLK_SCANCODE_MASK;
}

void initInput(InputState *theInput)
{
	*theInput = {};

	MemoryArena *systemArena = &globalAppState.systemArena;

	theInput->textEntered = makeString(&theInput->_textEntered[0], SDL_TEXTINPUTEVENT_TEXT_SIZE);
	theInput->textEnteredLength = 0;

	// Key names
	initHashTable(&theInput->keyNames, 0.75f, SDL_NUM_SCANCODES);

	// Letters
	for (char c = 'A'; c <= 'Z'; c++)
	{
		String key = pushString(systemArena, 1);
		key[0] = c;
		theInput->keyNames.put(key, (SDL_Keycode)(SDLK_a + (c - 'A')));
	}

	// Numbers
	for (char i = 0; i <= 9; i++)
	{
		String key = pushString(systemArena, 1);
		key[0] = '0' + i;
		theInput->keyNames.put(key, (SDL_Keycode)(SDLK_0 + i));
	}

	// F keys
	for (char i = 0; i <= 12; i++)
	{
		String key = pushString(systemArena, myprintf("F{0}"_s, {formatInt(i+1)}));
		theInput->keyNames.put(key, (SDL_Keycode)(SDLK_F1 + i));
	}

	// Misc
	theInput->keyNames.put(pushString(systemArena, "Home"), (SDL_Keycode)SDLK_HOME);
}

void updateInput()
{
	DEBUG_FUNCTION_T(DCDT_Input);

	// Clear mouse state
	inputState->wheelX = 0;
	inputState->wheelY = 0;

	// Fill-in "was down" info.
	copyMemory(inputState->mouseDown, inputState->mouseWasDown, MouseButtonCount);
	// NB: We don't want to simply ping-pong buffers here, because keyDown records whether it's down NOW,
	// and NOT whether it changed! I wasted some time doing ping-pong and being confused about it. /fp
	// - Sam, 10/07/2019
	copyMemory(inputState->_keyDown, inputState->_keyWasDown, KEYBOARD_KEY_COUNT);

	inputState->hasUnhandledTextEntered = false;
	inputState->textEnteredLength = 0;

	inputState->receivedQuitSignal = false;

	{
		DEBUG_BLOCK_T("updateInput: Pump", DCDT_Input);
		SDL_PumpEvents();
	}
	SDL_Event event;
	while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0)
	{
		DEBUG_BLOCK_T("updateInput: Event", DCDT_Input);

		switch (event.type) {
			// WINDOW EVENTS
			case SDL_QUIT: {
				inputState->receivedQuitSignal = true;
			} break;
			case SDL_WINDOWEVENT: {
				handleWindowEvent(&event.window);
			} break;

			// MOUSE EVENTS
			// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
			case SDL_MOUSEMOTION: {
				// Mouse pos in screen coordinates
				inputState->mousePosRaw.x = event.motion.x;
				inputState->mousePosRaw.y = event.motion.y;
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				u8 buttonIndex = event.button.button;
				inputState->mouseDown[buttonIndex] = true;
			} break;
			case SDL_MOUSEBUTTONUP: {
				u8 buttonIndex = event.button.button;
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
				inputState->_keyDownIsRepeat[keycode] = (event.key.repeat > 0);
				if (event.key.repeat)
				{
					// Slight hack: We detect a key press if _keyDown is true and _keyWasDown is false.
					// This hides repeats!
					// So, we falsely say _keyWasDown is false, so the repeat gets noticed.
					// You can distinguish between repeats and normal presses with _keyDownIsRepeat.
					// keyJustPressed() also has a parameter for ignoring repeats.
					inputState->_keyWasDown[keycode] = false;
				}
			} break;
			case SDL_KEYUP: {
				s32 keycode = keycodeToIndex(event.key.keysym.sym);
				inputState->_keyDown[keycode] = false;
				inputState->_keyDownIsRepeat[keycode] = false;
			} break;
			case SDL_TEXTINPUT: {
				inputState->hasUnhandledTextEntered = true;
				inputState->textEnteredLength = truncate32(strlen(event.text.text));
				copyString(event.text.text, inputState->textEnteredLength, &inputState->textEntered);
			} break;
		}
	}

	inputState->mousePosNormalised.x = ((inputState->mousePosRaw.x * 2.0f) / renderer->windowWidth) - 1.0f;
	inputState->mousePosNormalised.y = ((inputState->mousePosRaw.y * -2.0f) + renderer->windowHeight) / renderer->windowHeight;

	for (s32 i = 1; i < MouseButtonCount; i++)
	{
		MouseButton button = MouseButton(i);
		if (mouseButtonJustPressed(button))
		{
			// Store the initial click position
			inputState->clickStartPosNormalised[button] = inputState->mousePosNormalised;
		}
	}
}

/**
 * MOUSE INPUT
 */
inline bool mouseButtonJustPressed(MouseButton mouseButton) {
	return inputState->mouseDown[mouseButton] && !inputState->mouseWasDown[mouseButton];
}
inline bool mouseButtonJustReleased(MouseButton mouseButton) {
	return !inputState->mouseDown[mouseButton] && inputState->mouseWasDown[mouseButton];
}
inline bool mouseButtonPressed(MouseButton mouseButton) {
	return inputState->mouseDown[mouseButton];
}

inline V2 getClickStartPos(MouseButton mouseButton, struct Camera *camera)
{
	return unproject(camera, inputState->clickStartPosNormalised[mouseButton]);
}

/**
 * KEYBOARD INPUT
 */

inline bool modifierKeyIsPressed(ModifierKey modifier)
{
	bool result = false;

	switch (modifier)
	{
	case KeyMod_Alt:
		result = (inputState->_keyDown[keycodeToIndex(SDLK_LALT)] || inputState->_keyDown[keycodeToIndex(SDLK_RALT)]);
		break;
	case KeyMod_Ctrl:
		result = (inputState->_keyDown[keycodeToIndex(SDLK_LCTRL)] || inputState->_keyDown[keycodeToIndex(SDLK_RCTRL)]);
		break;
	case KeyMod_Shift:
		result = (inputState->_keyDown[keycodeToIndex(SDLK_LSHIFT)] || inputState->_keyDown[keycodeToIndex(SDLK_RSHIFT)]);
		break;
	case KeyMod_Super:
		result = (inputState->_keyDown[keycodeToIndex(SDLK_LGUI)] || inputState->_keyDown[keycodeToIndex(SDLK_RGUI)]);
		break;
	}

	return result;
}

inline u8 getPressedModifierKeys()
{
	u8 result = 0;

	if (modifierKeyIsPressed(KeyMod_Alt))    result |= KeyMod_Alt;
	if (modifierKeyIsPressed(KeyMod_Ctrl))   result |= KeyMod_Ctrl;
	if (modifierKeyIsPressed(KeyMod_Shift))  result |= KeyMod_Shift;
	if (modifierKeyIsPressed(KeyMod_Super))  result |= KeyMod_Super;

	return result;
}

inline bool modifierKeysArePressed(u8 modifiers)
{
	bool result = true;

	if (modifiers)
	{
		if (modifiers & KeyMod_Alt)
		{
			result = result && (inputState->_keyDown[keycodeToIndex(SDLK_LALT)] || inputState->_keyDown[keycodeToIndex(SDLK_RALT)]);
		}
		if (modifiers & KeyMod_Ctrl)
		{
			result = result && (inputState->_keyDown[keycodeToIndex(SDLK_LCTRL)] || inputState->_keyDown[keycodeToIndex(SDLK_RCTRL)]);
		}
		if (modifiers & KeyMod_Shift)
		{
			result = result && (inputState->_keyDown[keycodeToIndex(SDLK_LSHIFT)] || inputState->_keyDown[keycodeToIndex(SDLK_RSHIFT)]);
		}
		if (modifiers & KeyMod_Super)
		{
			result = result && (inputState->_keyDown[keycodeToIndex(SDLK_LGUI)] || inputState->_keyDown[keycodeToIndex(SDLK_RGUI)]);
		}
	}

	return result;
}

inline bool keyIsPressed(SDL_Keycode key, u8 modifiers)
{
	s32 keycode = keycodeToIndex(key);

	bool result = inputState->_keyDown[keycode];

	if (modifiers)
	{
		result = result && modifierKeysArePressed(modifiers);
	}

	return result;
}

inline bool keyWasPressed(SDL_Keycode key, u8 modifiers)
{
	s32 keycode = keycodeToIndex(key);

	bool result = inputState->_keyWasDown[keycode];

	if (modifiers)
	{
		if (modifiers & KeyMod_Alt)
		{
			result = result && (keyWasPressed(SDLK_LALT) || keyWasPressed(SDLK_RALT));
		}
		if (modifiers & KeyMod_Ctrl)
		{
			result = result && (keyWasPressed(SDLK_LCTRL) || keyWasPressed(SDLK_RCTRL));
		}
		if (modifiers & KeyMod_Shift)
		{
			result = result && (keyWasPressed(SDLK_LSHIFT) || keyWasPressed(SDLK_RSHIFT));
		}
		if (modifiers & KeyMod_Super)
		{
			result = result && (keyWasPressed(SDLK_LGUI) || keyWasPressed(SDLK_RGUI));
		}
	}

	return result;
}

inline bool keyJustPressed(SDL_Keycode key, u8 modifiers, bool ignoreRepeats)
{
	bool result = keyIsPressed(key, modifiers) && !keyWasPressed(key);

	if (result && ignoreRepeats)
	{
		s32 keycode = keycodeToIndex(key);

		// To ignore the repeat, we just return the opposite of whether it's a repeat.
		// The result is always true before this line.
		result = !inputState->_keyDownIsRepeat[keycode];
	}

	return result;
}

inline bool wasShortcutJustPressed(KeyboardShortcut shortcut)
{
	return keyJustPressed(shortcut.key, shortcut.modifiers, true);
}

inline bool wasTextEntered()
{
	return inputState->hasUnhandledTextEntered;
}

inline String getEnteredText()
{
	inputState->hasUnhandledTextEntered = false;
	return makeString(inputState->textEntered.chars, inputState->textEnteredLength);
}

inline String getClipboardText()
{
	String result = {};

	if (SDL_HasClipboardText())
	{
		char *clipboard = SDL_GetClipboardText();

		if (clipboard)
		{
			result = pushString(tempArena, clipboard);
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

	while (!isEmpty(keyName))
	{
		// 
		// MODIFIERS
		// 
		if (equals(keyName, "Alt"_s))
		{
			result.modifiers |= KeyMod_Alt;
		}
		else if (equals(keyName, "Ctrl"_s))
		{
			result.modifiers |= KeyMod_Ctrl;
		}
		else if (equals(keyName, "Shift"_s))
		{
			result.modifiers |= KeyMod_Shift;
		}
		else if (equals(keyName, "Super"_s))
		{
			result.modifiers |= KeyMod_Super;
		}
		else
		{
			Maybe<SDL_Keycode> found = inputState->keyNames.findValue(keyName);
			if (found.isValid)
			{
				result.key = found.value;
			}
			else
			{
				// Error!
				logWarn("Unrecognised key name '{0}' in shortcut string '{1}'"_s, {keyName, shortcutString});
				result.key = SDLK_UNKNOWN;
			}
			break;
		}

		keyName = nextToken(remainder, &remainder, '+');
	}

	return result;
}

void captureInput(void *target)
{
	inputState->capturedInputTarget = target;
}

void releaseInput(void *target)
{
	if (inputState->capturedInputTarget == target)
	{
		inputState->capturedInputTarget = null;
	}
}

bool hasCapturedInput(void *target)
{
	return inputState->capturedInputTarget == target;
}

bool isInputCaptured()
{
	return inputState->capturedInputTarget != null;
}
