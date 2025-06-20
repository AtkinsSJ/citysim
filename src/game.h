/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "AppStatus.h"
#include "city.h"
#include "game_clock.h"
#include "tile_utils.h"
#include "zone.h"
#include <Sim/BuildingRef.h>
#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/Flags.h>
#include <Util/Random.h>
#include <Util/String.h>
#include <Util/Vector.h>

struct Camera;

enum GameStatus {
    GameStatus_Playing,
    GameStatus_Quit,
};

enum GameMenuID {
    Menu_None,
    Menu_Build,
    Menu_Zone,
    Menu_System,
    Menu_DataViews,
};

enum ActionMode {
    ActionMode_None = 0,

    ActionMode_Build,
    ActionMode_Demolish,
    ActionMode_Zone,

    ActionMode_SetTerrain,

    ActionMode_Debug_AddFire,
    ActionMode_Debug_RemoveFire,

    ActionMode_Count,
};

struct DragState {
    bool isDragging;
    V2I mouseDragStartWorldPos;
    V2I mouseDragEndWorldPos;
};

enum DragType {
    DragLine,
    DragRect
};

enum DragResultOperation {
    DragResult_Nothing,
    DragResult_DoAction,
    DragResult_ShowPreview,
};

struct DragResult {
    DragResultOperation operation;
    Rect2I dragRect;
};

enum DataView {
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

struct DataViewUI {
    String title;

    // Gradient
    bool hasGradient;
    String gradientPaletteName;

    // Fixed-colors
    bool hasFixedColors;
    String fixedPaletteName;
    Array<String> fixedColorNames;

    // Overlay
    ChunkedArray<BuildingRef>* highlightedBuildings;
    EffectRadius BuildingDef::* effectRadiusMember;
    String overlayPaletteName;
    // NB: This is a pointer to the variable, not pointer to the array itself!
    // The DataViewUI data gets initialised before a City exists, so the per-tile arrays
    // have not been allocated yet.
    u8** overlayTileData;

    u8 (*calculateTileValue)(City* city, s32 x, s32 y);
};

enum InspectTileDebugFlags {
    DebugInspect_Fire,
    DebugInspect_Power,
    DebugInspect_Transport,

    InspectTileDebugFlagCount,
};

struct GameState {
    MemoryArena gameArena;
    GameStatus status;
    Random gameRandom;
    GameClock gameClock;
    City city;

    DataView dataLayerToDraw;
    DataViewUI dataViewUI[DataViewCount];

    DragState worldDragState;
    ActionMode actionMode;
    union {
        s32 selectedBuildingTypeID;
        ZoneType selectedZoneID;
        u8 selectedTerrainID;
    };

    // NB: This only works because we've made the inspect window unique! If we want to have multiple
    // at once, we'll need to figure out where to dynamically store the tile positions.
    // Honestly, I'd like to do that now anyway, but I can't think of a good way to do so.
    // - Sam, 11/2/2019
    V2I inspectedTilePosition;
    Flags<InspectTileDebugFlags, u8> inspectTileDebugFlags;
};

GameState* newGameState(); // A blank game state with no city
void beginNewGame();       // A game state for a new map

AppStatus updateAndRenderGame(GameState* gameState, f32 deltaTime);
void freeGameState(GameState* gameState);

void inputMoveCamera(Camera* camera, V2 windowSize, V2 windowMousePos, s32 cityWidth, s32 cityHeight);
void updateAndRenderGameUI(GameState* gameState);
void showCostTooltip(s32 buildCost);

Rect2I getDragArea(DragState* dragState, Rect2I cityBounds, DragType dragType, V2I itemSize);
DragResult updateDragState(DragState* dragState, Rect2I cityBounds, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize = { 1, 1 });

void drawDataViewOverlay(GameState* gameState, Rect2I visibleTileBounds);
void drawDataViewUI(GameState* gameState);

//
// Internal
//
void initDataViewUI(GameState* gameState);
void setGradient(DataViewUI* dataViewUI, String paletteName);
void setFixedColors(DataViewUI* dataView, String paletteName, std::initializer_list<String> names);
void setHighlightedBuildings(DataViewUI* dataViewUI, ChunkedArray<BuildingRef>* highlightedBuildings, EffectRadius BuildingDef::* effectRadiusMember = nullptr);
void setTileOverlay(DataViewUI* dataViewUI, u8** tileData, String paletteName);
void setTileOverlayCallback(DataViewUI* dataViewUI, u8 (*calculateTileValue)(City* city, s32 x, s32 y), String paletteName);

void inspectTileWindowProc(UI::WindowContext* context, void* userData);
void pauseMenuWindowProc(UI::WindowContext* context, void* userData);
void costTooltipWindowProc(UI::WindowContext* context, void* userData);
void debugToolsWindowProc(UI::WindowContext* context, void* userData);
