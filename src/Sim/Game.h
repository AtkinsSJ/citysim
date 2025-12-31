/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "AppStatus.h"
#include <Sim/BuildingRef.h>
#include <Sim/City.h>
#include <Sim/GameClock.h>
#include <Sim/TileUtils.h>
#include <Sim/Zone.h>
#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/EnumMap.h>
#include <Util/Flags.h>
#include <Util/Random.h>
#include <Util/String.h>
#include <Util/Vector.h>

enum class GameStatus : u8 {
    Playing,
    Quit,
};

enum class GameMenuID : u8 {
    None,
    Build,
    Zone,
    System,
    DataViews,
};

enum class ActionMode : u8 {
    None = 0,

    Build,
    Demolish,
    Zone,

    SetTerrain,

    Debug_AddFire,
    Debug_RemoveFire,
};

struct DragState {
    bool isDragging;
    V2I mouseDragStartWorldPos;
    V2I mouseDragEndWorldPos;
};

enum class DragType : u8 {
    Line,
    Rect
};

enum class DragResultOperation : u8 {
    Nothing,
    DoAction,
    ShowPreview,
};

struct DragResult {
    DragResultOperation operation;
    Rect2I dragRect;
};

enum class DataView : u8 {
    None,

    Desirability_Residential,
    Desirability_Commercial,
    Desirability_Industrial,

    Crime,
    Fire,
    Health,
    Pollution,
    Power,
    LandValue,

    COUNT
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

enum class InspectTileDebugFlags : u8 {
    Fire,
    Power,
    Transport,

    COUNT,
};

struct GameState {
    MemoryArena arena;
    GameStatus status;
    Random* gameRandom;
    GameClock gameClock;
    City city;

    DataView dataLayerToDraw;
    EnumMap<DataView, DataViewUI> dataViewUI;

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
    Flags<InspectTileDebugFlags> inspectTileDebugFlags;
};

GameState* newGameState(); // A blank game state with no city
void beginNewGame();       // A game state for a new map

AppStatus updateAndRenderGame(GameState* gameState, float deltaTime);
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
