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

	Rect2 contentArea;
	V2 currentOffset;
	f32 renderDepth;
	f32 perItemPadding;
};

typedef void (*WindowProc)(WindowContext*, void*);

enum WindowFlags
{
	WinFlag_AutomaticHeight = 0b00000001,
	WinFlag_Unique          = 0b00000010, // Only one window with the same WindowProc will be allowed. A new one will replace the old.
	WinFlag_Modal           = 0b00000100,
};

struct Window
{
	String title;
	u32 flags;

	Rect2I area;
	UIWindowStyle *style;

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

	u32 currentCursor;
	bool cursorIsVisible;
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows;
	bool isDraggingWindow;
	V2 windowDragMouseStartPos;
	V2 windowDragWindowStartPos;
};
