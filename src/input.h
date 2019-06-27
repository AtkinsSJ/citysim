#pragma once

const int KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;

enum MouseButton
{
	// NB: There is no button for 0, so any arrays of foo[MouseButtonCount] never use the first element.
	// I figure this is more straightforward than having to remember to subtract 1 from the button index,
	// or using different values than SDL does. "Wasting" one element is no big deal.
	// - Sam, 27/06/2019
	MouseButton_Left   = SDL_BUTTON_LEFT,
	MouseButton_Middle = SDL_BUTTON_MIDDLE,
	MouseButton_Right  = SDL_BUTTON_RIGHT,
	MouseButton_X1     = SDL_BUTTON_X1,
	MouseButton_X2     = SDL_BUTTON_X2,
	MouseButtonCount
};

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
	bool mouseDown[MouseButtonCount];
	bool mouseWasDown[MouseButtonCount];
	V2 clickStartPosNormalised[MouseButtonCount]; // In normalised (-1 to 1) coordinates
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

//
// PUBLIC
//
void initInput(InputState *inputState);
void updateInput(InputState *inputState);

bool mouseButtonJustPressed(InputState *input, MouseButton mouseButton);
bool mouseButtonJustReleased(InputState *input, MouseButton mouseButton);
bool mouseButtonPressed(InputState *input, MouseButton mouseButton);

V2 getClickStartPos(InputState *input, MouseButton mouseButton, struct Camera *camera);

bool modifierKeyIsPressed(InputState *input, ModifierKey modifier);
bool keyIsPressed(InputState *input, SDL_Keycode key, u8 modifiers=0);
bool keyWasPressed(InputState *input, SDL_Keycode key, u8 modifiers=0);
bool keyJustPressed(InputState *input, SDL_Keycode key, u8 modifiers=0);

KeyboardShortcut parseKeyboardShortcut(String input);
bool wasShortcutJustPressed(InputState *input, KeyboardShortcut shortcut);

bool wasTextEntered(InputState *input);
String getEnteredText(InputState *input);

String getClipboardText();

//
// INTERNAL
//

u32 keycodeToIndex(u32 key);
u8 getPressedModifierKeys(InputState *input);
bool modifierKeysArePressed(InputState *input, u8 modifiers);
