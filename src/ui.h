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
struct UIState
{
	UIMenuID openMenu;
	ActionMode actionMode;
	BuildingArchetype selectedBuildingArchetype;
};

const real32 messageDisplayTime = 2.0f;

#include "ui.cpp"