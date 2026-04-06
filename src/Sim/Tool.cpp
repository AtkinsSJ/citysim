/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Tool.h"

#include <Input/Input.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/Game.h>
#include <UI/Toast.h>
#include <UI/Window.h>
#include <Util/OwnedPtr.h>

Flags<InspectTool::DebugFlags> InspectTool::debug_flags;
V2I InspectTool::inspected_tile_pos;

NonnullOwnPtr<InspectTool> InspectTool::create()
{
    return adopt_own(*new InspectTool);
}

void InspectTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    if (!mouse_is_over_ui && mouseButtonJustPressed(MouseButton::Left)) {
        if (city.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {
            // FIXME: Pass this into the InspectionWindow constructor, or whatever, once that's a thing.
            InspectTool::inspected_tile_pos = mouse_tile_pos;

            V2I windowPos = v2i(the_renderer().ui_camera().mouse_position()) + v2i(16, 16);
            UI::showWindow(UI::WindowTitle::from_lambda([] {
                V2I tilePos = InspectTool::inspected_tile_pos;
                return getText("title_inspect"_s, { formatInt(tilePos.x), formatInt(tilePos.y) });
            }),
                250, 200, windowPos, "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Unique | WindowFlags::UniqueKeepPosition, inspectTileWindowProc, &city);
        }
    }
}

NonnullOwnPtr<BuildTool> BuildTool::create(BuildingType type)
{
    auto& def = *getBuildingDef(type);
    auto drag_type = def.buildMethod == BuildMethod::DragLine ? DragType::Line : DragType::Rect;
    return adopt_own(*new BuildTool(type, drag_type, def.size));
}

BuildTool::BuildTool(BuildingType type, DragType drag_type, V2I building_size)
    : m_building_type(type)
    , m_drag_state(drag_type, building_size)
{
}

void BuildTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    auto& renderer = the_renderer();
    auto ghostColorValid = Colour::from_rgb_255(128, 255, 128, 255);
    auto ghostColorInvalid = Colour::from_rgb_255(255, 0, 0, 128);

    BuildingDef* buildingDef = getBuildingDef(m_building_type);

    switch (buildingDef->buildMethod) {
    case BuildMethod::Paint: // Fallthrough
    case BuildMethod::Plop: {
        if (!mouse_is_over_ui) {
            Rect2I footprint = Rect2I::create_centre_size(mouse_tile_pos, buildingDef->size);
            s32 buildCost = buildingDef->buildCost;

            bool canPlace = city.can_place_building(buildingDef, footprint.x(), footprint.y());

            if ((buildingDef->buildMethod == BuildMethod::Plop && mouseButtonJustReleased(MouseButton::Left))
                || (buildingDef->buildMethod == BuildMethod::Paint && mouseButtonPressed(MouseButton::Left))) {
                if (canPlace && city.can_afford(buildCost)) {
                    city.place_building(buildingDef, footprint.x(), footprint.y());
                    city.spend(buildCost);
                }
            }

            showCostTooltip(buildCost);

            auto& sprite = Sprite::get(buildingDef->spriteName);
            auto color = canPlace ? ghostColorValid : ghostColorInvalid;
            drawSingleSprite(&renderer.world_overlay_buffer(), &sprite, footprint, renderer.shaderIds.pixelArt, color);
        }
    } break;

    case BuildMethod::DragLine: // Fallthrough
    case BuildMethod::DragRect: {
        auto [drag_operation, drag_rect] = m_drag_state.update(city.bounds, mouse_tile_pos, mouse_is_over_ui);
        s32 buildCost = city.calculate_build_cost(buildingDef, drag_rect);

        switch (drag_operation) {
        case DragResultOperation::DoAction: {
            if (city.can_afford(buildCost)) {
                city.place_building_rect(buildingDef, drag_rect);
                city.spend(buildCost);
            } else {
                UI::Toast::show(getText("msg_cannot_afford_construction"_s));
            }
        } break;

        case DragResultOperation::ShowPreview: {
            if (!mouse_is_over_ui)
                showCostTooltip(buildCost);

            if (city.can_afford(buildCost)) {
                auto& sprite = Sprite::get(buildingDef->spriteName);
                s32 maxGhosts = (drag_rect.width() / buildingDef->size.x) * (drag_rect.height() / buildingDef->size.y);
                // TODO: If maxGhosts is 1, just draw 1!
                DrawRectsGroup* rectsGroup = beginRectsGroupTextured(&renderer.world_overlay_buffer(), sprite.texture, renderer.shaderIds.pixelArt, maxGhosts);
                for (s32 y = 0; y + buildingDef->size.y <= drag_rect.height(); y += buildingDef->size.y) {
                    for (s32 x = 0; x + buildingDef->size.x <= drag_rect.width(); x += buildingDef->size.x) {
                        bool canPlace = city.can_place_building(buildingDef, drag_rect.x() + x, drag_rect.y() + y);

                        Rect2 rect { drag_rect.x() + x, drag_rect.y() + y, buildingDef->size.x, buildingDef->size.y };

                        auto color = canPlace ? ghostColorValid : ghostColorInvalid;
                        // TODO: All the sprites are the same, so we could optimise this!
                        // Then again, eventually we might want ghosts to not be identical, eg ghost roads that visually connect.
                        addSpriteRect(rectsGroup, &sprite, rect, color);
                    }
                }
                endRectsGroup(rectsGroup);
            } else {
                drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(255, 64, 64, 128));
            }
        } break;

        default:
            break;
        }
    } break;

        INVALID_DEFAULT_CASE;
    }
}

NonnullOwnPtr<DemolishTool> DemolishTool::create()
{
    return adopt_own(*new DemolishTool);
}

void DemolishTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    auto& renderer = the_renderer();
    auto [drag_operation, drag_rect] = m_drag_state.update(city.bounds, mouse_tile_pos, mouse_is_over_ui);
    s32 demolishCost = city.calculate_demolition_cost(drag_rect);
    city.demolitionRect = drag_rect;

    switch (drag_operation) {
    case DragResultOperation::DoAction: {
        if (city.can_afford(demolishCost)) {
            city.demolish_rect(drag_rect);
            city.spend(demolishCost);
        } else {
            UI::Toast::show(getText("msg_cannot_afford_demolition"_s));
        }
    } break;

    case DragResultOperation::ShowPreview: {
        if (!mouse_is_over_ui)
            showCostTooltip(demolishCost);

        if (city.can_afford(demolishCost)) {
            // Demolition outline
            drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(128, 0, 0, 128));
        } else {
            drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(255, 64, 64, 128));
        }
    } break;

    default:
        break;
    }
}

NonnullOwnPtr<ZoneTool> ZoneTool::create(ZoneType type)
{
    return adopt_own(*new ZoneTool(type));
}

ZoneTool::ZoneTool(ZoneType type)
    : m_zone_type(type)
{
}

void ZoneTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    auto& renderer = the_renderer();
    auto [drag_operation, drag_rect] = m_drag_state.update(city.bounds, mouse_tile_pos, mouse_is_over_ui);

    CanZoneQuery canZoneQuery = queryCanZoneTiles(&city, m_zone_type, drag_rect);
    s32 zoneCost = canZoneQuery.calculate_zone_cost();

    switch (drag_operation) {
    case DragResultOperation::DoAction: {
        if (city.can_afford(zoneCost)) {
            placeZone(&city, m_zone_type, drag_rect);
            city.spend(zoneCost);
        }
    } break;

    case DragResultOperation::ShowPreview: {
        if (!mouse_is_over_ui)
            showCostTooltip(zoneCost);
        if (city.can_afford(zoneCost)) {
            Colour palette[] = {
                Colour::from_rgb_255(255, 0, 0, 16),
                ZONE_DEFS[m_zone_type].color
            };
            drawGrid(&renderer.world_overlay_buffer(), canZoneQuery.bounds, canZoneQuery.tileCanBeZoned, ReadonlySpan { 2, palette });
        } else {
            drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(255, 64, 64, 128));
        }
    } break;

    default:
        break;
    }
}

NonnullOwnPtr<SetTerrainTool> SetTerrainTool::create(TerrainType type)
{
    return adopt_own(*new SetTerrainTool(type));
}

SetTerrainTool::SetTerrainTool(TerrainType type)
    : m_terrain_type(type)
{
}

void SetTerrainTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    // Temporary click-and-drag, no-cost terrain editing
    // We probably want to make this better in several ways, and add a cost to it, and such
    if (!mouse_is_over_ui
        && mouseButtonPressed(MouseButton::Left)
        && city.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {

        city.terrainLayer.set_terrain_at(mouse_tile_pos.x, mouse_tile_pos.y, m_terrain_type);
    }
}

NonnullOwnPtr<DebugTool> DebugTool::create(Mode mode)
{
    return adopt_own(*new DebugTool(mode));
}

DebugTool::DebugTool(Mode mode)
    : m_mode(mode)
{
}

void DebugTool::act(City& city, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    switch (m_mode) {
    case Mode::AddFire: {
        if (!mouse_is_over_ui
            && mouseButtonJustPressed(MouseButton::Left)
            && city.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {

            city.fireLayer.start_fire_at(city, mouse_tile_pos.x, mouse_tile_pos.y);
        }
        break;
    }
    case Mode::RemoveFire: {
        if (!mouse_is_over_ui
            && mouseButtonJustPressed(MouseButton::Left)
            && city.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {

            city.fireLayer.remove_fire_at(city, mouse_tile_pos.x, mouse_tile_pos.y);
        }
        break;
    }
    }
}
