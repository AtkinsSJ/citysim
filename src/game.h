#pragma once

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
	Menu_DataViews,
};

enum ActionMode
{
	ActionMode_None = 0,

	ActionMode_Build,
	ActionMode_Demolish,
	ActionMode_Zone,

	ActionMode_Debug_AddFire,
	ActionMode_Debug_RemoveFire,

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

enum DataView
{
	DataView_None,

	DataView_Desirability_Residential,
	DataView_Desirability_Commercial,
	DataView_Desirability_Industrial,

	DataView_Crime,
	DataView_Fire,
	DataView_Health,
	DataView_Pollution,
	DataView_Power,
	DataView_LandValue,

	DataViewCount
};

struct DataViewUI
{
	String title;

	// Gradient
	bool hasGradient;
	V4 gradientColorMin;
	V4 gradientColorMax;

	// Fixed-colors
	s32 fixedColorCount;
	V4 fixedColors[16];
	String fixedColorNames[16];
};

DataViewUI dataViewUI[DataViewCount] = {};

void initDataViewUI();

enum InspectTileDebugFlags
{
	DebugInspect_Fire,
	DebugInspect_Power,
	DebugInspect_Transport,

	InspectTileDebugFlagCount,
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	DataView dataLayerToDraw;
	DataViewUI dataLayerUI[DataViewCount];

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
	Flags<InspectTileDebugFlags, u8> inspectTileDebugFlags;
};

GameState *beginNewGame();
AppStatus updateAndRenderGame(GameState *gameState, UIState *uiState);
void freeGameState(GameState *gameState);

void inputMoveCamera(Camera *camera, V2 windowSize, V2 windowMousePos, s32 cityWidth, s32 cityHeight);
void updateAndRenderGameUI(UIState *uiState, GameState *gameState);
void showCostTooltip(UIState *uiState, s32 buildCost);

Rect2I getDragArea(DragState *dragState, DragType dragType, V2I itemSize);
DragResult updateDragState(DragState *dragState, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize = {1,1});

//
// Internal
//
void inspectTileWindowProc(WindowContext *context, void *userData);
void pauseMenuWindowProc(WindowContext *context, void *userData);
void costTooltipWindowProc(WindowContext *context, void *userData);
void debugToolsWindowProc(WindowContext *context, void *userData);
