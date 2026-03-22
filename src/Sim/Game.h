/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <App/Scene.h>
#include <Menus/SavedGames.h>
#include <Sim/BuildingRef.h>
#include <Sim/City.h>
#include <Sim/Zone.h>
#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/EnumMap.h>
#include <Util/Flags.h>
#include <Util/Random.h>
#include <Util/Ref.h>
#include <Util/String.h>
#include <Util/Vector.h>

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
    Array2<u8>* overlayTileData;

    using CalculateTileValue = Function<u8(City*, s32 x, s32 y)>;
    CalculateTileValue calculate_tile_value;
};

struct GameState {
    OwnPtr<Random> gameRandom;
    City city;

    DataView dataLayerToDraw { DataView::None };
    EnumMap<DataView, DataViewUI> dataViewUI;

    DragState worldDragState;
    ActionMode actionMode { ActionMode::None };
    union {
        s32 selectedBuildingTypeID;
        ZoneType selectedZoneID;
        u8 selectedTerrainID;
    };
};

void inputMoveCamera(Camera* camera, V2 windowSize, V2 windowMousePos, s32 cityWidth, s32 cityHeight, float delta_time);
void updateAndRenderGameUI(GameState* gameState);
void showCostTooltip(s32 buildCost);

Rect2I getDragArea(DragState* dragState, Rect2I cityBounds, DragType dragType, V2I itemSize);
DragResult updateDragState(DragState* dragState, Rect2I cityBounds, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize = { 1, 1 });

void drawDataViewOverlay(GameState* gameState, Rect2I visibleTileBounds);
void drawDataViewUI(GameState* gameState);

//
// Internal
//
void setGradient(DataViewUI* dataViewUI, String paletteName);
void setFixedColors(DataViewUI* dataView, String paletteName, std::initializer_list<String> names, MemoryArena&);
void setHighlightedBuildings(DataViewUI* dataViewUI, ChunkedArray<BuildingRef>* highlightedBuildings, EffectRadius BuildingDef::* effectRadiusMember = nullptr);
void setTileOverlay(DataViewUI* dataViewUI, Array2<u8>* tileData, String paletteName);
void setTileOverlayCallback(DataViewUI* dataViewUI, DataViewUI::CalculateTileValue, String paletteName);

void inspectTileWindowProc(UI::WindowContext* context, void* userData);
void pauseMenuWindowProc(UI::WindowContext* context, void* userData);
void costTooltipWindowProc(UI::WindowContext* context, void* userData);
void debugToolsWindowProc(UI::WindowContext* context, void* userData);

class GameScene final : public Scene {
public:
    static NonnullOwnPtr<GameScene> create_new(u32 seed);
    static ErrorOr<NonnullOwnPtr<GameScene>> from_saved_game(SavedGameInfo const&);

    virtual ~GameScene() override;
    virtual void update_and_render(float delta_time) override;

    MemoryArena& arena() { return m_arena; }
    GameState& state() { return m_state; }

private:
    GameScene();

    void init_data_view_ui();

    MemoryArena m_arena { "Game"_s };
    Ref<GameState> m_state;
};
