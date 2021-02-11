#pragma once

// NB: A WindowContext only exists for a single call of a WindowProc.
// It isn't kept around between update and render, so attempting to save
// state here that you want to carry over from one to the other will not work!!!
// AKA, I messed up.
struct WindowContext
{
	WindowContext(struct Window *window, struct UIWindowStyle *windowStyle, UIState *uiState, bool doUpdate, bool doRender);

	UIState *uiState;
	Window *window;
	UIWindowStyle *windowStyle;

	bool doUpdate;
	bool doRender;

	UIPanel windowPanel;

	// Results
	bool closeRequested = false;
};

typedef void (*WindowProc)(WindowContext*, void*);

enum WindowFlags
{
	WinFlag_AutomaticHeight = 1 << 0, // Height determined by content
	WinFlag_Unique          = 1 << 1, // Only one window with the same WindowProc will be allowed. A new one will replace the old.
	WinFlag_Modal           = 1 << 2,
	WinFlag_Tooltip         = 1 << 3, // Follows the mouse cursor
	WinFlag_Headless        = 1 << 4, // No title bar
	WinFlag_ShrinkWidth     = 1 << 5, // Reduces the width to match the content of the window
	WinFlag_Pause           = 1 << 6, // Pauses the game while open
};

struct Window
{
	String title;
	u32 flags;

	Rect2I area;
	String styleName;

	WindowProc windowProc;
	void *userData;
	WindowProc onClose;

	bool isInitialised;
	bool wasActiveLastUpdate;
};

//
// PUBLIC
//
void showWindow(UIState *uiState, String title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData, WindowProc onClose=null);

void updateWindows(UIState *uiState);
void renderWindows(UIState *uiState);

//
// INTERNAL
//
static void makeWindowActive(UIState *uiState, s32 windowIndex);
static Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 contentPadding);

WindowContext makeWindowContext(Window *window, UIWindowStyle *windowStyle, UIState *uiState, bool doUpdate, bool doRender);
