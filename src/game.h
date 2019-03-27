#pragma once

#include "city.h"

const int gameStartFunds = 1000000;

enum GameStatus
{
	GameStatus_Playing,
	GameStatus_Quit,
};

enum GameMenuID
{
	Menu_None,
	Menu_Build,
	Menu_Zone,
	Menu_System,
};

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Zone,

	ActionMode_Count,
};

struct DragState {
	bool isDragging;
	V2I mouseDragStartWorldPos;
	V2I mouseDragEndWorldPos;
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	u32 dataLayerToDraw;

	DragState worldDragState;
	ActionMode actionMode;
	union
	{
		s32 selectedBuildingTypeID;
		ZoneType selectedZoneID;
	};

	// NB: This only works because we've made the inspect window unique! If we want to have multiple
	// at once, we'll need to figure out where to dynamically store the tile positions.
	// Honestly, I'd like to do that now anyway, but I can't think of a good way to do so.
	// - Sam, 11/2/2019
	V2I inspectedTilePosition;
};