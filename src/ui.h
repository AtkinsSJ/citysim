#pragma once
// ui.h

struct Tooltip
{
	bool show;
	String styleName;
	String text;
	V2 offsetFromCursor;
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

typedef void (*WindowProc)(WindowContext*, struct Window*, void*);

struct Window
{
	String title;

	Rect2I area;
	bool hasAutomaticHeight;
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

	// Window stuff
	ChunkedArray<Window> openWindows;
	bool isDraggingWindow;
	V2 windowDragMouseStartPos;
	V2 windowDragWindowStartPos;
};

