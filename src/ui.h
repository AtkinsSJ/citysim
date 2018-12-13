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

	UITheme *theme;

	UIMenuID openMenu;
	ActionMode actionMode;
	union
	{
		u32 selectedBuildingTypeID;
		enum ZoneType selectedZoneID;
	};

	Tooltip tooltip;
	UiMessage message;

	Array<Rect2> uiRects;

	u32 currentCursor;
	bool cursorIsVisible;

	V2 mouseDragStartPos;
	Rect2I dragRect;
};

const f32 messageDisplayTime = 2.0f;

void setCursor(UIState *uiState, AssetManager *assets, u32 cursorID)
{
	uiState->currentCursor = cursorID;
	SDL_SetCursor(getCursor(assets, cursorID)->sdlCursor);
}