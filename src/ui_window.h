#pragma once

struct WindowContext
{
	struct UIState *uiState;
	MemoryArena *temporaryMemory;
	struct Window *window;
	UIWindowStyle *windowStyle;

	bool measureOnly; // Whether we're actually displaying/updating the window contents, or just measuring it

	Rect2 contentArea;
	V2 currentOffset;
	f32 largestItemWidth;
	s32 alignment;
	f32 renderDepth;
	f32 perItemPadding;

	// Results
	bool closeRequested;
};

typedef void (*WindowProc)(WindowContext*, void*);

enum WindowFlags
{
	WinFlag_AutomaticHeight = 1 << 0,
	WinFlag_Unique          = 1 << 1, // Only one window with the same WindowProc will be allowed. A new one will replace the old.
	WinFlag_Modal           = 1 << 2,
	WinFlag_Tooltip         = 1 << 3,
	WinFlag_Headless        = 1 << 4,
	WinFlag_ShrinkWidth     = 1 << 5,
};

struct Window
{
	String title;
	u32 flags;

	Rect2I area;
	String styleName;

	WindowProc windowProc;
	void *userData;
};

void showWindow(UIState *uiState, String title, s32 width, s32 height, String styleName, u32 flags, WindowProc windowProc, void *userData);

void window_label(WindowContext *context, String text, char *styleName=null);
/*
 * If you pass textWidth, then the button will be sized as though the text was that size. If you leave
 * it blank (or pass -1 manually) then the button will be automatically sized to wrap the contained text.
 * Either way, it always matches the size vertically.
 */
bool window_button(WindowContext *context, String text, s32 textWidth=-1);

void updateAndRenderWindows(UIState *uiState);