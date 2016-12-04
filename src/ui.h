#pragma once
// ui.h

enum BuildingArchetype; // Defined in city.h

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Plant,
	ActionMode_Harvest,
	ActionMode_Hire,

	ActionMode_Count,
};

enum UIMenuID
{
	UIMenu_None,
	UIMenu_Build,
};

struct Tooltip
{
	bool show;
	V2 offsetFromCursor;
	V4 color;
	char text[256];
};
const real32 uiMessageBottomMargin = 4,
			uiMessageTextPadding = 4;
struct UiMessage
{
	char text[256];
	real32 countdown; // In seconds
};

struct UIState
{
	MemoryArena arena;

	UIMenuID openMenu;
	ActionMode actionMode;
	BuildingArchetype selectedBuildingArchetype;

	Tooltip tooltip;
	UiMessage message;

	RealRect uiRects[32];
	int32 uiRectCount;

	CursorType currentCursor;
	bool cursorIsVisible;
};

const real32 messageDisplayTime = 2.0f;