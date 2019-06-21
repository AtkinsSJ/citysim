#pragma once

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

struct DragState
{
	V2I citySize;
	bool isDragging;
	V2I mouseDragStartWorldPos;
	V2I mouseDragEndWorldPos;
};

enum DragType
{
	DragLine,
	DragRect
};

enum DragResultOperation
{
	DragResult_Nothing,
	DragResult_DoAction,
	DragResult_ShowPreview,
};

struct DragResult
{
	DragResultOperation operation;
	Rect2I dragRect;
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	// Rendering-related stuff
	u32 dataLayerToDraw;
	// Things we want to draw in world-space, over the top of the world, but which we know about before rendering.
	// eg, ghost building previews are calculated early in the game loop, but we want them drawn at the end.
	// 
	ChunkedArray<RenderItem> overlayRenderItems;

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

const V4 genericDataLayerColors[] = {
	color255(  0,   0, 255, 63),
	color255(  0, 255,   0, 63),
	color255(255,   0,   0, 63),
	color255(  0, 255, 255, 63),
	color255(255, 255,   0, 63),
	color255(255,   0, 255, 63)
};
const s32 genericDataLayerColorCount = 6;
