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
	String text;
};
const f32 uiMessageBottomMargin = 4,
			uiMessageTextPadding = 4;
struct UiMessage
{
	String text;
	f32 countdown; // In seconds
};

struct UIState
{
	MemoryArena arena;

	UIMenuID openMenu;
	ActionMode actionMode;
	BuildingArchetype selectedBuildingArchetype;

	Tooltip tooltip;
	UiMessage message;

	Rect2 uiRects[32];
	s32 uiRectCount;

	CursorType currentCursor;
	bool cursorIsVisible;

	V2 mouseDragStartPos;
	Rect2I dragRect;
};

const f32 messageDisplayTime = 2.0f;

void setCursor(UIState *uiState, AssetManager *assets, CursorType cursorID)
{
	uiState->currentCursor = cursorID;
	SDL_SetCursor(getCursor(assets, cursorID)->sdlCursor);
}