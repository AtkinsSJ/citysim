/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Game.h"

#include <App/App.h>
#include <Assets/AssetManager.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Palette.h>
#include <Gfx/Renderer.h>
#include <Input/Input.h>
#include <Menus/About.h>
#include <Menus/MainMenu.h>
#include <Menus/SaveFile.h>
#include <Menus/SavedGames.h>
#include <Settings/Settings.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/TerrainCatalogue.h>
#include <Sim/Tool.h>
#include <UI/Toast.h>
#include <UI/Window.h>
#include <Util/Random.h>

void GameScene::move_camera_from_input(Camera& camera, V2 window_size, V2 window_mouse_pos, float delta_time)
{
    DEBUG_FUNCTION();

    s32 const CAMERA_MARGIN = 1;          // How many tiles beyond the map can the camera scroll to show?
    float const CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second

    // Zooming
    s32 zoomDelta = input_state().wheelY;

    // Turns out that having the zoom bound to the same key I use for navigating debug frames is REALLY ANNOYING
    if (!isInputCaptured()) {
        if (keyJustPressed(SDLK_EQUALS) || keyJustPressed(SDLK_KP_PLUS)) {
            zoomDelta++;
        } else if (keyJustPressed(SDLK_MINUS) || keyJustPressed(SDLK_KP_MINUS)) {
            zoomDelta--;
        }
    }

    if (zoomDelta) {
        camera.zoom_by(zoomDelta * 0.1f);
    }

    // Panning
    float scrollSpeed = (CAMERA_PAN_SPEED * sqrt(camera.zoom())) * delta_time;
    float cameraEdgeScrollPixelMargin = 8.0f;

    if (mouseButtonPressed(MouseButton::Middle)) {
        // Click-panning!
        float scale = scrollSpeed * 1.0f;
        V2 clickStartPos = getClickStartPos(MouseButton::Middle, &camera);
        camera.move_by((camera.mouse_position() - clickStartPos) * scale);
    } else if (!isInputCaptured()) {
        if (keyIsPressed(SDLK_LEFT)
            || keyIsPressed(SDLK_a)
            || (window_mouse_pos.x < cameraEdgeScrollPixelMargin)) {
            camera.move_by(v2(-scrollSpeed, 0.f));
        } else if (keyIsPressed(SDLK_RIGHT)
            || keyIsPressed(SDLK_d)
            || (window_mouse_pos.x > (window_size.x - cameraEdgeScrollPixelMargin))) {
            camera.move_by(v2(scrollSpeed, 0.f));
        }

        if (keyIsPressed(SDLK_UP)
            || keyIsPressed(SDLK_w)
            || (window_mouse_pos.y < cameraEdgeScrollPixelMargin)) {
            camera.move_by(v2(0.f, -scrollSpeed));
        } else if (keyIsPressed(SDLK_DOWN)
            || keyIsPressed(SDLK_s)
            || (window_mouse_pos.y > (window_size.y - cameraEdgeScrollPixelMargin))) {
            camera.move_by(v2(0.f, scrollSpeed));
        }
    }

    // Clamp camera
    camera.snap_to_rectangle({ -CAMERA_MARGIN, -CAMERA_MARGIN, m_state->city.bounds.width() + (2 * CAMERA_MARGIN), m_state->city.bounds.height() + (2 * CAMERA_MARGIN) });
}

Rect2I getDragArea(DragState* dragState, Rect2I cityBounds, DragType dragType, V2I itemSize)
{
    DEBUG_FUNCTION();

    Rect2I result { 0, 0, 0, 0 };

    if (dragState->isDragging) {
        switch (dragType) {
        case DragType::Rect: {
            // This is more complicated than a simple rectangle covering both points.
            // If the user is dragging a 3x3 building, then the drag area must cover that 3x3 footprint
            // even if they drag right-to-left, and in the case where the rectangle is not an even multiple
            // of 3, the building placements are aligned to match that initial footprint.
            // I'm struggling to find a good way of describing that... But basically, we want this to be
            // as intuitive to use as possible, and that means there MUST be a building placed where the
            // ghost was before the user pressed the mouse button, no matter which direction or size they
            // then drag it!

            V2I startP = dragState->mouseDragStartWorldPos;
            V2I endP = dragState->mouseDragEndWorldPos;

            if (startP.x < endP.x) {
                auto raw_width = max(endP.x - startP.x + 1, itemSize.x);
                s32 xRemainder = raw_width % itemSize.x;
                result.set_x(startP.x);
                result.set_width(raw_width - xRemainder);
            } else {
                auto raw_width = startP.x - endP.x + itemSize.x;
                s32 xRemainder = raw_width % itemSize.x;
                result.set_x(endP.x + xRemainder);
                result.set_width(raw_width - xRemainder);
            }

            if (startP.y < endP.y) {
                auto raw_height = max(endP.y - startP.y + 1, itemSize.y);
                s32 yRemainder = raw_height % itemSize.y;
                result.set_y(startP.y);
                result.set_height(raw_height - yRemainder);
            } else {
                auto raw_height = startP.y - endP.y + itemSize.y;
                s32 yRemainder = raw_height % itemSize.y;
                result.set_y(endP.y + yRemainder);
                result.set_height(raw_height - yRemainder);
            }

        } break;

        case DragType::Line: {
            // Axis-aligned straight line, in one dimension.
            // So, if you drag a diagonal line, it picks which direction has greater length and uses that.

            V2I startP = dragState->mouseDragStartWorldPos;
            V2I endP = dragState->mouseDragEndWorldPos;

            // determine orientation
            s32 xDiff = abs(startP.x - endP.x);
            s32 yDiff = abs(startP.y - endP.y);

            if (xDiff > yDiff) {
                // X
                if (startP.x < endP.x) {
                    auto raw_width = max(xDiff + 1, itemSize.x);
                    s32 xRemainder = raw_width % itemSize.x;
                    result.set_x(startP.x);
                    result.set_width(raw_width - xRemainder);
                } else {
                    auto raw_width = xDiff + itemSize.x;
                    s32 xRemainder = raw_width % itemSize.x;
                    result.set_x(endP.x + xRemainder);
                    result.set_width(raw_width - xRemainder);
                }

                result.set_y(startP.y);
                result.set_height(itemSize.y);
            } else {
                // Y
                if (startP.y < endP.y) {
                    auto raw_height = max(yDiff + 1, itemSize.y);
                    s32 yRemainder = raw_height % itemSize.y;
                    result.set_y(startP.y);
                    result.set_height(raw_height - yRemainder);
                } else {
                    auto raw_height = yDiff + itemSize.y;
                    s32 yRemainder = raw_height % itemSize.y;
                    result.set_y(endP.y + yRemainder);
                    result.set_height(raw_height - yRemainder);
                }

                result.set_x(startP.x);
                result.set_width(itemSize.x);
            }
        } break;

            INVALID_DEFAULT_CASE;
        }

        result = result.intersected(cityBounds);
    }

    return result;
}

DragResult updateDragState(DragState* dragState, Rect2I cityBounds, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize)
{
    DEBUG_FUNCTION();

    DragResult result = {};

    if (dragState->isDragging && mouseButtonJustReleased(MouseButton::Left)) {
        if (!mouseIsOverUI) {
            result.operation = DragResultOperation::DoAction;
            result.dragRect = getDragArea(dragState, cityBounds, dragType, itemSize);
        }

        dragState->isDragging = false;
    } else {
        // Update the dragging state
        if (!mouseIsOverUI && mouseButtonJustPressed(MouseButton::Left)) {
            dragState->isDragging = true;
            dragState->mouseDragStartWorldPos = dragState->mouseDragEndWorldPos = mouseTilePos;
        }

        if (mouseButtonPressed(MouseButton::Left) && dragState->isDragging) {
            dragState->mouseDragEndWorldPos = mouseTilePos;
            result.dragRect = getDragArea(dragState, cityBounds, dragType, itemSize);
        } else {
            result.dragRect = { mouseTilePos.x, mouseTilePos.y, 1, 1 };
        }

        // Preview
        if (!mouseIsOverUI || dragState->isDragging) {
            result.operation = DragResultOperation::ShowPreview;
        }
    }

    // minimum size
    if (result.dragRect.width() < itemSize.x)
        result.dragRect.set_width(itemSize.x);
    if (result.dragRect.height() < itemSize.y)
        result.dragRect.set_height(itemSize.y);

    return result;
}

void inspectTileWindowProc(UI::WindowContext* context, void* userData)
{
    DEBUG_FUNCTION();

    UI::Panel* ui = &context->windowPanel;
    ui->alignWidgets(HAlign::Fill);

    City* city = static_cast<City*>(userData);

    V2I tilePos = city->inspectedTilePosition;

    // CitySector
    CitySector* sector = city->sectors.get_sector_at_tile_pos(tilePos.x, tilePos.y);
    ui->addLabel(myprintf("CitySector: x={0} y={1} w={2} h={3}"_s, { formatInt(sector->bounds.x()), formatInt(sector->bounds.y()), formatInt(sector->bounds.width()), formatInt(sector->bounds.height()) }));

    // Terrain
    auto& terrain = city->terrainLayer.terrain_at(tilePos.x, tilePos.y);
    ui->addLabel(myprintf("Terrain: {0}, {1} tiles from water"_s, { getText(terrain.textAssetName), formatInt(city->terrainLayer.distance_to_water_at(tilePos.x, tilePos.y)) }));

    // Zone
    ZoneType zone = city->zoneLayer.get_zone_at(tilePos.x, tilePos.y);
    ui->addLabel(myprintf("Zone: {0}"_s, { zone == ZoneType::None ? "None"_s : getText(ZONE_DEFS[zone].textAssetName) }));

    // Building
    Building* building = city->get_building_at(tilePos.x, tilePos.y);
    if (building != nullptr) {
        s32 buildingIndex = city->tileBuildingIndex.get(tilePos.x, tilePos.y);
        BuildingDef* def = getBuildingDef(building);
        ui->addLabel(myprintf("Building: {0} (ID {1}, array index {2})"_s, { getText(def->textAssetName), formatInt(building->id), formatInt(buildingIndex) }));
        ui->addLabel(myprintf("Constructed: {0}"_s, { formatDateTime(dateTimeFromTimestamp(building->creationDate), DateTimeFormat::ShortDate) }));
        ui->addLabel(myprintf("Variant: {0}"_s, { formatInt(building->variantIndex.value_or(-1)) }));
        ui->addLabel(myprintf("- Residents: {0} / {1}"_s, { formatInt(building->currentResidents), formatInt(def->residents) }));
        ui->addLabel(myprintf("- Jobs: {0} / {1}"_s, { formatInt(building->currentJobs), formatInt(def->jobs) }));
        ui->addLabel(myprintf("- Power: {0}"_s, { formatInt(def->power) }));

        // Problems
        for (auto problem_type : enum_values<BuildingProblem::Type>()) {
            if (hasProblem(building, problem_type)) {
                ui->addLabel(myprintf("- PROBLEM: {0}"_s, { getText(buildingProblemNames[problem_type]) }));
            }
        }
    } else {
        ui->addLabel("Building: None"_s);
    }

    // Land value
    ui->addLabel(myprintf("Land value: {0}%"_s, { formatFloat(city->landValueLayer.get_land_value_percent_at(tilePos.x, tilePos.y) * 100.0f, 0) }));

    // Debug info
    if (!city->inspectTileDebugFlags.is_empty()) {
        if (city->inspectTileDebugFlags.has(InspectTileDebugFlags::Fire)) {
            city->fireLayer.debug_inspect(*ui, tilePos, building);
        }
        if (city->inspectTileDebugFlags.has(InspectTileDebugFlags::Power)) {
            city->powerLayer.debug_inspect(*ui, tilePos);
        }
        if (city->inspectTileDebugFlags.has(InspectTileDebugFlags::Transport)) {
            city->transportLayer.debug_inspect(*ui, tilePos);
        }
    }

    // Highlight
    // Part of me wants this to happen outside of this windowproc, but we don't have a way of knowing when
    // the uiwindow is closed. Maybe at some point we'll want that functionality for other reasons, but
    // for now, it's cleaner and simpler to just do that drawing here.
    // Though, that does mean we can't control *when* the highlight is drawn, or make the building be drawn
    // as highlighted, so maybe this won't work and I'll have to delete this comment in 30 seconds' time!
    // - Sam, 28/08/2019

    auto tileHighlightColor = Colour::from_rgb_255(196, 196, 255, 64);
    auto& renderer = the_renderer();
    drawSingleRect(&renderer.world_overlay_buffer(), Rect2 { tilePos.x, tilePos.y, 1, 1 }, renderer.shaderIds.untextured, tileHighlightColor);
}

void pauseMenuWindowProc(UI::WindowContext* context, void* /*userData*/)
{
    DEBUG_FUNCTION();
    UI::Panel* ui = &context->windowPanel;
    ui->alignWidgets(HAlign::Fill);

    if (ui->addTextButton(getText("button_resume"_s))) {
        context->closeRequested = true;
    }

    if (ui->addTextButton(getText("button_save"_s))) {
        showSaveGameWindow();
    }

    if (ui->addTextButton(getText("button_load"_s))) {
        showLoadGameWindow();
    }

    if (ui->addTextButton(getText("button_about"_s))) {
        showAboutWindow();
    }

    if (ui->addTextButton(getText("button_settings"_s))) {
        showSettingsWindow();
    }

    if (ui->addTextButton(getText("button_exit"_s))) {
        App::the().switch_to_scene(MainMenuScene::create());
    }
}

void GameScene::update_and_render_game_ui()
{
    DEBUG_FUNCTION();

    auto& renderer = the_renderer();
    RenderBuffer* uiBuffer = &renderer.ui_buffer();
    auto& label_style = UI::LabelStyle::get("title"_s);
    auto& font = label_style.font.get();
    City* city = &m_state->city;

    s32 const uiPadding = 4; // TODO: Move this somewhere sensible!
    s32 left = uiPadding;
    s32 right = UI::windowSize.x - uiPadding;
    s32 rowHeight = 30;
    s32 toolbarHeight = uiPadding * 2 + rowHeight * 2;
    s32 width3 = (UI::windowSize.x - (uiPadding * 2)) / 3;

    Rect2I uiRect { 0, 0, UI::windowSize.x, toolbarHeight };
    UI::addUIRect(uiRect);
    drawSingleRect(uiBuffer, uiRect, renderer.shaderIds.untextured, Colour::from_rgb_255(0, 0, 0, 128));

    UI::putLabel(city->name, { left, uiPadding, width3, rowHeight }, &label_style);

    UI::putLabel(myprintf("£{0} (-£{1}/month)"_s, { formatInt(city->funds), formatInt(city->monthlyExpenditure) }), { width3, uiPadding, width3, rowHeight }, &label_style);

    UI::putLabel(myprintf("Pop: {0}, Jobs: {1}"_s, { formatInt(city->zoneLayer.total_residents()), formatInt(city->zoneLayer.total_jobs()) }), { width3, uiPadding + rowHeight, width3, rowHeight }, &label_style);

    // Game clock
    Rect2I clockBounds = {};
    {
        GameClock* clock = &city->gameClock;

        // We're sizing the clock area based on the speed control buttons.
        // The >>> button is the largest, so they're all set to that size.
        // For the total area, we just add their widths and padding together.
        auto& button_style = UI::ButtonStyle::get("default"_s);
        V2I speedButtonSize = UI::calculateButtonSize(">>>"_s, &button_style);
        s32 clockWidth = (speedButtonSize.x * 4) + (uiPadding * 3);
        clockBounds = { right - clockWidth, uiPadding, clockWidth, toolbarHeight };

        String dateString = formatDateTime(clock->cosmetic_date(), DateTimeFormat::ShortDate);
        V2I dateStringSize = font.calculate_text_size(dateString, clockWidth);

        // Draw a progress bar for the current day
        Rect2I dateRect { right - clockWidth, uiPadding, clockWidth, dateStringSize.y };
        drawSingleRect(uiBuffer, dateRect, renderer.shaderIds.untextured, Colour::from_rgb_255(0, 0, 0, 128));
        Rect2I dateProgressRect = dateRect;
        dateProgressRect.set_width(round_s32(dateProgressRect.width() * clock->current_day_completion()));
        drawSingleRect(uiBuffer, dateProgressRect, renderer.shaderIds.untextured, Colour::from_rgb_255(64, 255, 64, 128));

        UI::putLabel(dateString, dateRect, &label_style);

        // Speed control buttons
        Rect2I speedButtonRect { right - speedButtonSize.x, toolbarHeight - (uiPadding + speedButtonSize.y), speedButtonSize.x, speedButtonSize.y };

        if (UI::putTextButton(">>>"_s, speedButtonRect, &button_style, buttonIsActive(clock->speed() == GameClockSpeed::Fast))) {
            clock->set_speed(GameClockSpeed::Fast);
            clock->set_is_paused(false);
        }
        speedButtonRect.set_x(speedButtonRect.x() - (speedButtonRect.width() + uiPadding));

        if (UI::putTextButton(">>"_s, speedButtonRect, &button_style, buttonIsActive(clock->speed() == GameClockSpeed::Medium))) {
            clock->set_speed(GameClockSpeed::Medium);
            clock->set_is_paused(false);
        }
        speedButtonRect.set_x(speedButtonRect.x() - (speedButtonRect.width() + uiPadding));

        if (UI::putTextButton(">"_s, speedButtonRect, &button_style, buttonIsActive(clock->speed() == GameClockSpeed::Slow))) {
            clock->set_speed(GameClockSpeed::Slow);
            clock->set_is_paused(false);
        }
        speedButtonRect.set_x(speedButtonRect.x() - (speedButtonRect.width() + uiPadding));

        if (UI::putTextButton("||"_s, speedButtonRect, &button_style, buttonIsActive(clock->is_paused()))) {
            clock->set_is_paused(!clock->is_paused());
        }
    }

    /*
            UI::putLabel(myprintf("Power: {0}/{1}"_s, {formatInt(city->powerLayer.cachedCombinedConsumption), formatInt(city->powerLayer.cachedCombinedProduction)}),
                   irectXYWH(right - width3, uiPadding, width3, rowHeight), labelStyle);
    */

    UI::putLabel(myprintf("R: {0}\nC: {1}\nI: {2}"_s, { formatInt(city->zoneLayer.demand[ZoneType::Residential]), formatInt(city->zoneLayer.demand[ZoneType::Commercial]), formatInt(city->zoneLayer.demand[ZoneType::Industrial]) }),
        { clockBounds.x() - 100, uiPadding, 100, toolbarHeight }, &label_style);

    auto& button_style = UI::ButtonStyle::get("default"_s);
    auto& popup_menu_panel_style = UI::PanelStyle::get("popupMenu"_s);
    // Build UI
    {
        // The, um, "MENU" menu. Hmmm.
        String menuButtonText = getText("button_menu"_s);
        V2I buttonSize = UI::calculateButtonSize(menuButtonText, &button_style);
        Rect2I buttonRect { uiPadding, toolbarHeight - (buttonSize.y + uiPadding), buttonSize.x, buttonSize.y };
        if (UI::putTextButton(menuButtonText, buttonRect, &button_style)) {
            UI::showWindow(UI::WindowTitle::from_text_asset("title_menu"_s), 200, 200, v2i(0, 0), "default"_s,
                WindowFlags::Unique | WindowFlags::Modal | WindowFlags::AutomaticHeight | WindowFlags::Pause,
                pauseMenuWindowProc);
        }
        buttonRect.set_x(buttonRect.x() + buttonRect.width() + uiPadding);

        // The "ZONE" menu
        String zoneButtonText = getText("button_zone"_s);
        buttonRect.set_size(UI::calculateButtonSize(zoneButtonText, &button_style));
        if (UI::putTextButton(zoneButtonText, buttonRect, &button_style, buttonIsActive(UI::isMenuVisible(to_underlying(GameMenuID::Zone))))) {
            UI::toggleMenuVisible(to_underlying(GameMenuID::Zone));
        }

        if (UI::isMenuVisible(to_underlying(GameMenuID::Zone))) {
            s32 popupMenuWidth = 150;
            s32 popupMenuMaxHeight = UI::windowSize.y - (buttonRect.y() + buttonRect.height());

            UI::Panel menu = UI::Panel({ buttonRect.x() - popup_menu_panel_style.padding.left, buttonRect.y() + buttonRect.height(), popupMenuWidth, popupMenuMaxHeight }, &popup_menu_panel_style);
            for (auto zone_type : enum_values<ZoneType>()) {
                auto is_active = [&] {
                    if (auto* zone_tool = dynamic_cast<ZoneTool*>(m_active_tool.ptr()); zone_tool && zone_tool->zone_type() == zone_type)
                        return ButtonState::Active;
                    return ButtonState::Normal;
                }();
                if (menu.addTextButton(getText(ZONE_DEFS[zone_type].textAssetName), is_active)) {
                    UI::hideMenus();
                    set_active_tool(ZoneTool::create(zone_type));
                    renderer.set_cursor("build"_s);
                }
            }
            menu.end(true);
        }

        buttonRect.set_x(buttonRect.x() + buttonRect.width() + uiPadding);

        // The "BUILD" menu
        String buildButtonText = getText("button_build"_s);
        buttonRect.set_size(UI::calculateButtonSize(buildButtonText, &button_style));
        if (UI::putTextButton(buildButtonText, buttonRect, &button_style, buttonIsActive(UI::isMenuVisible(to_underlying(GameMenuID::Build))))) {
            UI::toggleMenuVisible(to_underlying(GameMenuID::Build));
        }

        if (UI::isMenuVisible(to_underlying(GameMenuID::Build))) {
            auto& constructibleBuildings = BuildingCatalogue::the().constructible_buildings();

            s32 popupMenuWidth = 150;
            s32 popupMenuMaxHeight = UI::windowSize.y - (buttonRect.y() + buttonRect.height());

            UI::Panel menu = UI::Panel({ buttonRect.x() - popup_menu_panel_style.padding.left, buttonRect.y() + buttonRect.height(), popupMenuWidth, popupMenuMaxHeight }, &popup_menu_panel_style);

            for (auto it = constructibleBuildings.iterate();
                it.hasNext();
                it.next()) {
                BuildingDef* buildingDef = it.getValue();

                auto is_active = [&] {
                    if (auto* build_tool = dynamic_cast<BuildTool*>(m_active_tool.ptr()); build_tool && build_tool->building_type() == buildingDef->typeID)
                        return ButtonState::Active;
                    return ButtonState::Normal;
                }();

                if (menu.addTextButton(getText(buildingDef->textAssetName), is_active)) {
                    UI::hideMenus();
                    set_active_tool(BuildTool::create(buildingDef->typeID));
                    renderer.set_cursor("build"_s);
                }
            }

            menu.end(true);
        }
        buttonRect.set_x(buttonRect.x() + buttonRect.width() + uiPadding);

        // The Terrain button
        String terrainButtonText = getText("button_terrain"_s);
        buttonRect.set_size(UI::calculateButtonSize(terrainButtonText, &button_style));
        bool isTerrainWindowOpen = UI::isWindowOpen(modify_terrain_window_proc);
        if (UI::putTextButton(terrainButtonText, buttonRect, &button_style, buttonIsActive(isTerrainWindowOpen))) {
            if (isTerrainWindowOpen) {
                UI::closeWindow(modify_terrain_window_proc);
            } else {
                show_terrain_window();
            }
        }
        buttonRect.set_x(buttonRect.x() + buttonRect.width() + uiPadding);

        // Demolish button
        String demolishButtonText = getText("button_demolish"_s);
        buttonRect.set_size(UI::calculateButtonSize(demolishButtonText, &button_style));
        if (UI::putTextButton(demolishButtonText, buttonRect, &button_style,
                buttonIsActive(dynamic_cast<DemolishTool*>(m_active_tool.ptr()) != nullptr))) {
            set_active_tool(DemolishTool::create());
            renderer.set_cursor("demolish"_s);
        }
        buttonRect.set_x(buttonRect.x() + buttonRect.width() + uiPadding);
    }

    draw_data_view_ui();
}

void costTooltipWindowProc(UI::WindowContext* context, void* userData)
{
    s32 buildCost = truncate32((smm)userData);
    City* city = &App::the().game_state()->city;

    UI::Panel* ui = &context->windowPanel;

    auto style = city->can_afford(buildCost)
        ? "cost-affordable"_sv
        : "cost-unaffordable"_sv;

    String text = myprintf("£{0}"_s, { formatInt(buildCost) });
    ui->addLabel(text, style);
}

void showCostTooltip(s32 buildCost)
{
    UI::showTooltip(costTooltipWindowProc, (void*)(smm)buildCost);
}

void debugToolsWindowProc(UI::WindowContext* context, void* userData)
{
    auto* game_scene = dynamic_cast<GameScene*>(&App::the().scene());
    if (!game_scene) {
        context->closeRequested = true;
        return;
    }

    GameState* gameState = (GameState*)userData;
    City& city = gameState->city;
    UI::Panel* ui = &context->windowPanel;
    ui->alignWidgets(HAlign::Fill);

    if (ui->addTextButton("Inspect fire info"_s, buttonIsActive(city.inspectTileDebugFlags.has(InspectTileDebugFlags::Fire)))) {
        city.inspectTileDebugFlags.toggle(InspectTileDebugFlags::Fire);
    }
    auto debug_tool_is_active = [&](DebugTool::Mode mode) {
        if (auto* debug_tool = dynamic_cast<DebugTool const*>(&game_scene->active_tool()); debug_tool && debug_tool->mode() == mode)
            return ButtonState::Active;
        return ButtonState::Normal;
    };
    if (ui->addTextButton("Add Fire"_s, debug_tool_is_active(DebugTool::Mode::AddFire))) {
        game_scene->set_active_tool(DebugTool::create(DebugTool::Mode::AddFire));
    }
    if (ui->addTextButton("Remove Fire"_s, debug_tool_is_active(DebugTool::Mode::RemoveFire))) {
        game_scene->set_active_tool(DebugTool::create(DebugTool::Mode::RemoveFire));
    }

    if (ui->addTextButton("Inspect power info"_s, buttonIsActive(city.inspectTileDebugFlags.has(InspectTileDebugFlags::Power)))) {
        city.inspectTileDebugFlags.toggle(InspectTileDebugFlags::Power);
    }

    if (ui->addTextButton("Inspect transport info"_s, buttonIsActive(city.inspectTileDebugFlags.has(InspectTileDebugFlags::Transport)))) {
        city.inspectTileDebugFlags.toggle(InspectTileDebugFlags::Transport);
    }
}

NonnullOwnPtr<GameScene> GameScene::create_new(u32 seed)
{
    auto& app_state = App::the();

    auto game_scene = adopt_own(*new GameScene);

    game_scene->m_state->gameRandom = Random::create(seed);
    app_state.set_game_state(&*game_scene->m_state);

    s32 gameStartFunds = 1000000;
    initCity(&game_scene->m_arena, &game_scene->m_state->city, 128, 128, getText("city_default_name"_s), getText("player_default_name"_s), gameStartFunds);
    game_scene->m_state->city.terrainLayer.generate(game_scene->m_state->city, seed);

    return game_scene;
}

ErrorOr<NonnullOwnPtr<GameScene>> GameScene::from_saved_game(SavedGameInfo const& saved_game_info)
{
    auto& app_state = App::the();

    auto game_scene = adopt_own(*new GameScene);

    auto& game_state = *game_scene->m_state;
    // FIXME: Replace this fixed seed once we're not in dev mode.
    game_state.gameRandom = Random::create(12345);

    FileHandle saveFile = openFile(saved_game_info.fullPath, FileAccessMode::Read);
    bool loadSucceeded = [&] {
        // So... I'm not really sure how to signal success, honestly.
        // I suppose the process ouytside of this function is:
        // - User confirms to load a city.
        // - Existing city, if any, is discarded.
        // - This function is called.
        // - If it fails, discard the city, else it's in memory.
        // So, if loading fails, then no city will be in memory, regardless of whether one was
        // before the loading was attempted! I think that makes the most sense.
        // Another option would be to load into a second City struct, and then swap it if it
        // successfully loads... but that makes a bunch of memory-management more complicated.
        // This way, we only ever have one City in memory so we can clean up easily.

        // For now, reading the whole thing into memory and then processing it is simpler.
        // However, it's wasteful memory-wise, so if save files get big we might want to
        // read the file a bit at a time. @Size

        bool succeeded = false;

        City* city = &game_state.city;

        BinaryFileReader reader = readBinaryFile(&saveFile, SAV_FILE_ID, &temp_arena());
        // This doesn't actually loop, we're just using a `while` so we can break out of it
        while (reader.isValidFile) {
            // META
            bool readMeta = reader.startSection(SAV_META_ID, SAV_META_VERSION);
            if (readMeta) {
                SAVSection_Meta* meta = reader.readStruct<SAVSection_Meta>(0);

                String cityName = reader.readString(meta->cityName);
                String playerName = reader.readString(meta->playerName);
                initCity(&game_scene->m_arena, city, meta->cityWidth, meta->cityHeight, cityName, playerName, meta->funds, meta->currentDate, meta->timeWithinDay);

                // Camera
                auto& world_camera = the_renderer().world_camera();
                world_camera.set_position(v2(meta->cameraX, meta->cameraY));
                world_camera.set_zoom(meta->cameraZoom);
            } else
                break;

            if (!city->terrainLayer.load(reader))
                break;
            if (!city->load_buildings(&reader))
                break;
            if (!city->zoneLayer.load(reader))
                break;

            bool any_city_layer_failed_to_load = false;
            for (auto& layer : city->m_layers) {
                if (!layer->load(reader, *city)) {
                    any_city_layer_failed_to_load = true;
                    break;
                }
            }

            if (any_city_layer_failed_to_load)
                break;

            // And we're done!
            succeeded = true;
            break;
        }

        return succeeded;
    }();
    closeFile(&saveFile);

    if (!loadSucceeded)
        return getText("msg_load_failure"_s, { saved_game_info.shortName });
    app_state.set_game_state(&game_state);
    return game_scene;
}

GameScene::GameScene()
    : m_state(*m_arena.allocate<GameState>())
    , m_active_tool(InspectTool::create())
{
    init_data_view_ui();
    // FIXME: Set cursor for InspectTool
}

GameScene::~GameScene() = default;

void GameScene::update_and_render(float delta_time)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    auto& renderer = the_renderer();
    City& city = m_state->city;

    // Update the simulation... need a smarter way of doing this!
    if (!UI::hasPauseWindowOpen()) {
        DEBUG_BLOCK_T("Update simulation", DebugCodeDataTag::Simulation);

        auto clockEvents = city.gameClock.increment(delta_time);
        if (clockEvents.has(ClockEvents::NewWeek)) {
            logInfo("New week!"_s);
        }
        if (clockEvents.has(ClockEvents::NewMonth)) {
            logInfo("New month, a budget tick should happen here!"_s);
        }
        if (clockEvents.has(ClockEvents::NewYear)) {
            logInfo("New year!"_s);
        }

        city.update();
    }

    // UI!
    update_and_render_game_ui();

    // CAMERA!
    Camera& world_camera = renderer.world_camera();
    Camera& ui_camera = renderer.ui_camera();
    move_camera_from_input(world_camera, ui_camera.size(), ui_camera.mouse_position(), delta_time);

    V2I mouseTilePos = v2i(world_camera.mouse_position());
    bool mouseIsOverUI = UI::isMouseInputHandled() || UI::mouseIsWithinUIRects();

    city.demolitionRect = Rect2I::create_negative_infinity();

    {
        DEBUG_BLOCK_T("Tool update", DebugCodeDataTag::GameUpdate);
        m_active_tool->act(city, mouseIsOverUI, mouseTilePos);
    }

    if (mouseButtonJustPressed(MouseButton::Right)) {
        // Switch to inspect tool
        if (dynamic_cast<InspectTool*>(m_active_tool.ptr()) == nullptr) {
            set_active_tool(InspectTool::create());
            renderer.set_cursor("default"_s);
        }
    }

    // RENDERING
    // Pre-calculate the tile area that's visible to the player.
    // We err on the side of drawing too much, rather than risking having holes in the world.
    Rect2I visibleTileBounds = Rect2I::create_centre_size(
        v2i(world_camera.position()), v2i(world_camera.size() / world_camera.zoom()) + v2i(3, 3));
    visibleTileBounds = visibleTileBounds.intersected(city.bounds);

    // logInfo("visibleTileBounds = {0} {1} {2} {3}"_s, {formatInt(visibleTileBounds.x),formatInt(visibleTileBounds.y),formatInt(visibleTileBounds.w),formatInt(visibleTileBounds.h)});

    city.draw(visibleTileBounds);

    // Data layer rendering
    if (m_state->dataLayerToDraw != DataView::None) {
        draw_data_view_overlay(visibleTileBounds);
    }
}

void GameScene::init_data_view_ui()
{
    auto& dataViewUI = m_state->dataViewUI;
    City* city = &m_state->city;

    dataViewUI[DataView::None].title = "data_view_none"_s;

    dataViewUI[DataView::Desirability_Residential].title = "data_view_desirability_residential"_s;
    setGradient(&dataViewUI[DataView::Desirability_Residential], "desirability"_s);
    setTileOverlay(&dataViewUI[DataView::Desirability_Residential], &city->zoneLayer.tileDesirability[ZoneType::Residential], "desirability"_s);

    dataViewUI[DataView::Desirability_Commercial].title = "data_view_desirability_commercial"_s;
    setGradient(&dataViewUI[DataView::Desirability_Commercial], "desirability"_s);
    setTileOverlay(&dataViewUI[DataView::Desirability_Commercial], &city->zoneLayer.tileDesirability[ZoneType::Commercial], "desirability"_s);

    dataViewUI[DataView::Desirability_Industrial].title = "data_view_desirability_industrial"_s;
    setGradient(&dataViewUI[DataView::Desirability_Industrial], "desirability"_s);
    setTileOverlay(&dataViewUI[DataView::Desirability_Industrial], &city->zoneLayer.tileDesirability[ZoneType::Industrial], "desirability"_s);

    dataViewUI[DataView::Crime].title = "data_view_crime"_s;
    setGradient(&dataViewUI[DataView::Crime], "service_coverage"_s);
    setFixedColors(&dataViewUI[DataView::Crime], "service_buildings"_s, { "data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s }, m_arena);
    setHighlightedBuildings(&dataViewUI[DataView::Crime], city->crimeLayer.police_buildings(), &BuildingDef::policeEffect);
    setTileOverlay(&dataViewUI[DataView::Crime], city->crimeLayer.tile_police_coverage(), "service_coverage"_s);

    dataViewUI[DataView::Fire].title = "data_view_fire"_s;
    setGradient(&dataViewUI[DataView::Fire], "risk"_s);
    setFixedColors(&dataViewUI[DataView::Fire], "service_buildings"_s, { "data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s }, m_arena);
    setHighlightedBuildings(&dataViewUI[DataView::Fire], city->fireLayer.fire_protection_buildings(), &BuildingDef::fireProtection);
    setTileOverlay(&dataViewUI[DataView::Fire], city->fireLayer.tile_overall_fire_risk(), "risk"_s);

    dataViewUI[DataView::Health].title = "data_view_health"_s;
    setGradient(&dataViewUI[DataView::Health], "service_coverage"_s);
    setFixedColors(&dataViewUI[DataView::Health], "service_buildings"_s, { "data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s }, m_arena);
    setHighlightedBuildings(&dataViewUI[DataView::Health], city->healthLayer.health_buildings(), &BuildingDef::healthEffect);
    setTileOverlay(&dataViewUI[DataView::Health], city->healthLayer.tile_health_coverage(), "service_coverage"_s);

    dataViewUI[DataView::LandValue].title = "data_view_landvalue"_s;
    setGradient(&dataViewUI[DataView::LandValue], "land_value"_s);
    setTileOverlay(&dataViewUI[DataView::LandValue], city->landValueLayer.tile_land_value(), "land_value"_s);

    dataViewUI[DataView::Pollution].title = "data_view_pollution"_s;
    setGradient(&dataViewUI[DataView::Pollution], "pollution"_s);
    setTileOverlay(&dataViewUI[DataView::Pollution], city->pollutionLayer.tile_pollution(), "pollution"_s);

    dataViewUI[DataView::Power].title = "data_view_power"_s;
    setFixedColors(&dataViewUI[DataView::Power], "power"_s, { "data_view_power_powered"_s, "data_view_power_brownout"_s, "data_view_power_blackout"_s }, m_arena);

    setHighlightedBuildings(&dataViewUI[DataView::Power], city->powerLayer.power_buildings());
    setTileOverlayCallback(&dataViewUI[DataView::Power], [](City* city, s32 x, s32 y) { return city->powerLayer.calculate_power_overlay_for_tile(x, y); }, "power"_s);
}

void setGradient(DataViewUI* dataView, String paletteName)
{
    dataView->hasGradient = true;
    dataView->gradientPaletteName = paletteName;
}

void setFixedColors(DataViewUI* dataView, String paletteName, std::initializer_list<String> names, MemoryArena& arena)
{
    dataView->hasFixedColors = true;
    dataView->fixedPaletteName = paletteName;
    dataView->fixedColorNames = arena.allocate_array<String>(truncate32(names.size()), false);
    for (auto it = names.begin(); it != names.end(); it++) {
        dataView->fixedColorNames.append(arena.allocate_string(*it));
    }
}

void setHighlightedBuildings(DataViewUI* dataView, ChunkedArray<BuildingRef>* highlightedBuildings, EffectRadius BuildingDef::* effectRadiusMember)
{
    dataView->highlightedBuildings = highlightedBuildings;
    dataView->effectRadiusMember = effectRadiusMember;
}

void setTileOverlay(DataViewUI* dataView, Array2<u8>* tileData, String paletteName)
{
    dataView->overlayTileData = tileData;
    dataView->overlayPaletteName = paletteName;
}

void setTileOverlayCallback(DataViewUI* dataView, DataViewUI::CalculateTileValue calculate_tile_value, String paletteName)
{
    dataView->calculate_tile_value = calculate_tile_value;
    dataView->overlayPaletteName = paletteName;
}

template<typename Iterable>
static void drawBuildingHighlights(City* city, Iterable* buildingRefs)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);
    auto& renderer = the_renderer();

    if (buildingRefs->count > 0) {
        auto& buildingsPalette = Palette::get("service_buildings"_s);
        s32 paletteIndexPowered = 0;
        s32 paletteIndexUnpowered = 1;

        DrawRectsGroup* buildingHighlights = beginRectsGroupUntextured(&renderer.world_overlay_buffer(), renderer.shaderIds.untextured, buildingRefs->count);
        for (auto it = buildingRefs->iterate(); it.hasNext(); it.next()) {
            Building* building = city->get_building(it.getValue());
            // NB: If we're doing this in a separate loop, we could crop out buildings that aren't in the visible tile bounds
            if (building != nullptr) {
                s32 paletteIndex = (buildingHasPower(building) ? paletteIndexPowered : paletteIndexUnpowered);
                addUntexturedRect(buildingHighlights, building->footprint, buildingsPalette.colour_at(paletteIndex));
            }
        }
        endRectsGroup(buildingHighlights);
    }
}

template<typename Iterable>
static void drawBuildingEffectRadii(City* city, Iterable* buildingRefs, EffectRadius BuildingDef::* effectMember)
{
    auto& renderer = the_renderer();
    //
    // Leaving a note here because it's the first time I've used a pointer-to-member, and it's
    // weird and confusing and the syntax is odd!
    //
    // You declare the variable/parameter like this:
    //    EffectRadius BuildingDef::* effectMember;
    // Essentially it's just a regular variable definition, with the struct name in the middle and ::*
    //
    // Then, we use it like this:
    //    EffectRadius *effect = &(def->*effectMember);
    // (The important part is      ^^^^^^^^^^^^^^^^^^)
    // So, you access the member like you would normally with a . or ->, except you put a * before
    // the member name to show that it's a member pointer instead of an actual member.
    //
    // Now that I've written it out, it's not so bad, but it was really hard to look up about because
    // I assumed this would be a template-related feature, and so all the examples I found were super
    // template heavy and abstract and confusing. But no! It's a built-in feature that's actually not
    // too complicated. I might use this more now that I know about it.
    //
    // - Sam, 18/09/2019
    //

    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    if (buildingRefs->count > 0) {
        auto& ringsPalette = Palette::get("coverage_radius"_s);
        s32 paletteIndexPowered = 0;
        s32 paletteIndexUnpowered = 1;

        DrawRingsGroup* buildingRadii = beginRingsGroup(&renderer.world_overlay_buffer(), buildingRefs->count);

        for (auto it = buildingRefs->iterate(); it.hasNext(); it.next()) {
            Building* building = city->get_building(it.getValue());
            // NB: We don't filter buildings outside of the visibleTileBounds because their radius might be
            // visible even if the building isn't!
            if (building != nullptr) {
                BuildingDef* def = getBuildingDef(building);
                EffectRadius* effect = &(def->*effectMember);
                if (effect->has_effect()) {
                    s32 paletteIndex = (buildingHasPower(building) ? paletteIndexPowered : paletteIndexUnpowered);
                    addRing(buildingRadii, building->footprint.centre(), static_cast<float>(effect->radius()), 0.5f, ringsPalette.colour_at(paletteIndex));
                }
            }
        }

        endRingsGroup(buildingRadii);
    }
}

void GameScene::draw_data_view_overlay(Rect2I visible_tile_bounds) const
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    if (m_state->dataLayerToDraw == DataView::None)
        return;
    ASSERT(to_underlying(m_state->dataLayerToDraw) < to_underlying(DataView::COUNT));
    auto& renderer = the_renderer();

    City* city = &m_state->city;
    DataViewUI* dataView = &m_state->dataViewUI[m_state->dataLayerToDraw];

    if (dataView->overlayTileData) {
        // TODO: Use the visible tile bounds for rendering instead. We have two paths, one is just to output
        // an array in the layer as a texture, which we use for most things. The other is to generate an array
        // dynamically, which we only calculate for the visibleTileBounds to save unnecessary work.
        // The array-as-texture path currently returns the entire map, because that was simplest, but that
        // wastefully uploads the entire map's data to the GPU each frame anyway, so it's inefficient, even
        // though cropping it is non-trivial. But yeah, if they are consistent that would make things easier
        // to follow.
        // - Sam, 28/03/2020
        Rect2I bounds = city->bounds;

        auto& overlayPalette = Palette::get(dataView->overlayPaletteName);
        drawGrid(&renderer.world_overlay_buffer(), bounds, *dataView->overlayTileData, (u16)overlayPalette.size(), overlayPalette.raw_colour_data());
    } else if (dataView->calculate_tile_value) {
        // The per-tile overlay data is generated
        Array2<u8> overlayTileData = temp_arena().allocate_array_2d<u8>(visible_tile_bounds.size());

        for (s32 gridY = 0; gridY < visible_tile_bounds.height(); gridY++) {
            for (s32 gridX = 0; gridX < visible_tile_bounds.width(); gridX++) {
                u8 tileValue = dataView->calculate_tile_value(city, visible_tile_bounds.x() + gridX, visible_tile_bounds.y() + gridY);
                overlayTileData.set(gridX, gridY, tileValue);
            }
        }

        auto& overlayPalette = Palette::get(dataView->overlayPaletteName);
        drawGrid(&renderer.world_overlay_buffer(), visible_tile_bounds, overlayTileData, (u16)overlayPalette.size(), overlayPalette.raw_colour_data());
    }

    if (dataView->highlightedBuildings) {
        drawBuildingHighlights(city, dataView->highlightedBuildings);

        if (dataView->effectRadiusMember) {
            drawBuildingEffectRadii(city, dataView->highlightedBuildings, dataView->effectRadiusMember);
        }
    }
}

void GameScene::draw_data_view_ui() const
{
    DEBUG_FUNCTION();

    auto& renderer = the_renderer();
    RenderBuffer* uiBuffer = &renderer.ui_buffer();
    auto& label_style = UI::LabelStyle::get("title"_s);
    auto& font = label_style.font.get();

    s32 const uiPadding = 4; // TODO: Move this somewhere sensible!
    auto& button_style = UI::ButtonStyle::get("default"_s);
    auto& popup_menu_panel_style = UI::PanelStyle::get("popupMenu"_s);

    // Data-views menu
    String dataViewButtonText = getText("button_data_views"_s);
    V2I dataViewButtonSize = UI::calculateButtonSize(dataViewButtonText, &button_style);
    Rect2I dataViewButtonBounds { uiPadding, UI::windowSize.y - uiPadding - dataViewButtonSize.y, dataViewButtonSize.x, dataViewButtonSize.y };

    Rect2I dataViewUIBounds = dataViewButtonBounds.expanded(uiPadding);
    drawSingleRect(uiBuffer, dataViewUIBounds, renderer.shaderIds.untextured, Colour::from_rgb_255(0, 0, 0, 128));
    UI::addUIRect(dataViewUIBounds);

    if (UI::putTextButton(dataViewButtonText, dataViewButtonBounds, &button_style)) {
        UI::toggleMenuVisible(to_underlying(GameMenuID::DataViews));
    }

    if (UI::isMenuVisible(to_underlying(GameMenuID::DataViews))) {
        // Measure the menu contents
        UI::ButtonStyle& popupButtonStyle = popup_menu_panel_style.buttonStyle.get();
        s32 buttonMaxWidth = 0;
        s32 buttonMaxHeight = 0;
        for (auto data_view : enum_values<DataView>()) {
            String buttonText = getText(m_state->dataViewUI[data_view].title);
            V2I buttonSize = UI::calculateButtonSize(buttonText, &popupButtonStyle);
            buttonMaxWidth = max(buttonMaxWidth, buttonSize.x);
            buttonMaxHeight = max(buttonMaxHeight, buttonSize.y);
        }
        s32 popupMenuWidth = buttonMaxWidth + popup_menu_panel_style.padding.left + popup_menu_panel_style.padding.right;
        s32 popupMenuMaxHeight = UI::windowSize.y - 128;
        s32 estimatedMenuHeight = (to_underlying(DataView::COUNT) * buttonMaxHeight)
            + ((to_underlying(DataView::COUNT) - 1) * popup_menu_panel_style.contentPadding)
            + (popup_menu_panel_style.padding.top + popup_menu_panel_style.padding.bottom);

        UI::Panel menu = UI::Panel(Rect2I::create_aligned(dataViewButtonBounds.x() - popup_menu_panel_style.padding.left, dataViewButtonBounds.y(), popupMenuWidth, popupMenuMaxHeight, { HAlign::Left, VAlign::Bottom }), &popup_menu_panel_style, UI::PanelFlags::LayoutBottomToTop);

        // Enable scrolling if there's too many items to fit
        if (estimatedMenuHeight > popupMenuMaxHeight) {
            menu.enableVerticalScrolling(UI::getMenuScrollbar(), true);
        }

        // FIXME: Reversed iteration somehow
        for (auto data_view : enum_values<DataView>()) {
            String buttonText = getText(m_state->dataViewUI[data_view].title);

            if (menu.addTextButton(buttonText, buttonIsActive(m_state->dataLayerToDraw == data_view))) {
                UI::hideMenus();
                m_state->dataLayerToDraw = data_view;
            }
        }

        menu.end(true);
    }

    // Data-view info
    if (!UI::isMenuVisible(to_underlying(GameMenuID::DataViews))
        && m_state->dataLayerToDraw != DataView::None) {
        V2I uiPos = dataViewButtonBounds.position();
        uiPos.y -= uiPadding;

        DataViewUI* dataView = &m_state->dataViewUI[m_state->dataLayerToDraw];

        s32 paletteBlockSize = font.line_height();

        UI::Panel ui = UI::Panel(Rect2I::create_aligned(uiPos.x, uiPos.y, 300, 1000, { HAlign::Left, VAlign::Bottom }), nullptr, UI::PanelFlags::LayoutBottomToTop);
        {
            // We're working from bottom to top, so we start at the end.

            // First, the named colors
            if (dataView->hasFixedColors) {
                auto& fixedPalette = Palette::get(dataView->fixedPaletteName);
                ASSERT(fixedPalette.size() >= dataView->fixedColorNames.count);

                for (s32 fixedColorIndex = dataView->fixedColorNames.count - 1; fixedColorIndex >= 0; fixedColorIndex--) {
                    Rect2I paletteBlockBounds = ui.addBlank(paletteBlockSize, paletteBlockSize);
                    drawSingleRect(uiBuffer, paletteBlockBounds, renderer.shaderIds.untextured, fixedPalette.colour_at(fixedColorIndex).as_opaque());

                    ui.addLabel(getText(dataView->fixedColorNames[fixedColorIndex]));
                    ui.startNewLine();
                }
            }

            // Above that, the gradient
            if (dataView->hasGradient) {
                // Arbitrarily going to make the height 4x the width
                s32 gradientHeight = paletteBlockSize * 4;
                UI::Panel gradientPanel = ui.row(gradientHeight, VAlign::Bottom, "plain"_sv);

                UI::Panel gradientColumn = gradientPanel.column(paletteBlockSize, HAlign::Left, "plain"_sv);
                {
                    Rect2I gradientBounds = gradientColumn.addBlank(paletteBlockSize, gradientHeight);

                    auto& gradientPalette = Palette::get(dataView->gradientPaletteName);
                    auto minColor = gradientPalette.first().as_opaque();
                    auto maxColor = gradientPalette.last().as_opaque();

                    drawSingleRect(uiBuffer, gradientBounds, renderer.shaderIds.untextured, maxColor, maxColor, minColor, minColor);
                }
                gradientColumn.end();

                gradientPanel.addLabel(getText("data_view_minimum"_s));

                // @Hack! We read some internal Panel fields to manually move the "max" label up
                s32 spaceHeight = (gradientHeight - (2 * gradientPanel.largestItemHeightOnLine) - (2 * gradientPanel.style->contentPadding));
                gradientPanel.startNewLine();
                gradientPanel.addBlank(1, spaceHeight);
                gradientPanel.startNewLine();

                gradientPanel.addLabel(getText("data_view_maximum"_s));

                gradientPanel.end();
            }

            // Title and close button
            // TODO: Probably want to make this a Window that can't be moved?
            // Close button first to ensure it has space
            ui.alignWidgets(HAlign::Right);
            if (ui.addTextButton("X"_s)) {
                m_state->dataLayerToDraw = DataView::None;
            }
            ui.alignWidgets(HAlign::Left);
            ui.addLabel(getText(dataView->title), "title"_sv);
        }
        ui.end(true);
    }
}

void GameScene::set_active_tool(NonnullOwnPtr<Tool> tool)
{
    // TODO: Change cursors here
    m_active_tool = move(tool);
}
