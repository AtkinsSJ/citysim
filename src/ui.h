#pragma once
// ui.h

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Zone,

	ActionMode_Count,
};

enum UIMenuID
{
	UIMenu_None,
	UIMenu_Build,
	UIMenu_Zone,
	UIMenu_System,
};

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

/* 
 * So... Really, we have two different things here:
 * - Global UI state, such as the theme and current cursor
 * - Game-specific stuff, like what the action mode is, drag state, etc.
 * Probably they should be separated out. UI element code in one place, and game-ui logic elsewhere.
 * BUT, I don't know exactly what should go where, and I don't want to fiddle with it right now.
 *
 * - Sam, 14/12/2018
 */
struct UIState
{
	MemoryArena arena;
	RenderBuffer *uiBuffer;
	AssetManager *assets;
	InputState *input;

	UIMenuID openMenu;
	ActionMode actionMode;
	union
	{
		u32 selectedBuildingTypeID;
		enum ZoneType selectedZoneID;
	};

	f32 closestDepth;

	Tooltip tooltip;
	UiMessage message;

	Array<Rect2> uiRects;

	u32 currentCursor;
	bool cursorIsVisible;

	// These are a bit awkwardly named, they should probably go in a game-ui struct instead.
	bool isWorldDragging;
	V2I mouseDragStartWorldPos;
	V2I mouseDragEndWorldPos;

	// Window stuff
	ChunkedArray<Window> openWindows;
	bool isDraggingWindow;
	V2 windowDragMouseStartPos;
	V2 windowDragWindowStartPos;
};

