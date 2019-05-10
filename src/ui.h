#pragma once
// ui.h

struct Tooltip
{
	bool show;
	String styleName;
	String text;
};

const f32 uiMessageDisplayTime = 2.0f;
const f32 uiMessageBottomMargin = 4;
const f32 uiMessageTextPadding = 4;
struct UiMessage
{
	String text;
	f32 countdown; // In seconds
};

struct Window;
struct WindowContext
{
	struct UIState *uiState;
	MemoryArena *temporaryMemory;
	Window *window;
	UIWindowStyle *windowStyle;

	bool measureOnly; // Whether we're actually displaying/updating the window contents, or just measuring it

	Rect2 contentArea;
	V2 currentOffset;
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

struct UIState
{
	MemoryArena arena;
	RenderBuffer *uiBuffer;
	AssetManager *assets;
	InputState *input;

	f32 closestDepth;

	Tooltip tooltip;
	UiMessage message;

	// TODO: Replace this with better "this input has already been used" code!
	Array<Rect2> uiRects;

	s32 openMenu;

	String currentCursor;
	bool cursorIsVisible;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows;
	bool isDraggingWindow;
	V2 windowDragWindowStartPos;
};

void setCursor(UIState *uiState, String cursorName)
{
	Asset *newCursorAsset = getAsset(uiState->assets, AssetType_Cursor, cursorName);
	if (newCursorAsset != null)
	{
		uiState->currentCursor = cursorName;
		SDL_SetCursor(newCursorAsset->cursor.sdlCursor);
	}
}
inline void setCursor(UIState *uiState, char *cursorName)
{
	setCursor(uiState, makeString(cursorName));
}