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

enum DataLayer
{
	DataLayer_None,

	DataLayer_Desirability_Residential,
	DataLayer_Desirability_Commercial,
	DataLayer_Desirability_Industrial,

	DataLayer_Crime,
	DataLayer_FireRisk,
	DataLayer_Health,
	DataLayer_Pollution,
	DataLayer_Power,
	DataLayer_LandValue,

	DataLayerCount
};

struct GameState
{
	MemoryArena gameArena;
	GameStatus status;
	Random gameRandom;
	City city;

	// Rendering-related stuff
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
