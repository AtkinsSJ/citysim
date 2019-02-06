#pragma once

#include "city.h"

const int gameStartFunds = 1000000;

enum GameStatus
{
	GameStatus_Playing,
	GameStatus_Quit,
};

enum ActionMode {
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Zone,

	ActionMode_Count,
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	u32 dataLayerToDraw;

	// These are a bit awkwardly named, they should probably go in a game-ui struct instead.
	bool isWorldDragging;
	V2I mouseDragStartWorldPos;
	V2I mouseDragEndWorldPos;

	ActionMode actionMode;
	union
	{
		u32 selectedBuildingTypeID;
		ZoneType selectedZoneID;
	};
};