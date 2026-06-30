/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Tool.h"

#include <Input/Input.h>
#include <Sim/Basic.h>
#include <Sim/Budget.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/Game.h>
#include <Sim/TerrainCatalogue.h>
#include <UI/Toast.h>
#include <UI/Window.h>
#include <Util/OwnedPtr.h>

Flags<InspectTool::DebugFlags> InspectTool::debug_flags;
V2I InspectTool::inspected_tile_pos;

OwnedRef<InspectTool> InspectTool::create()
{
    return adopt_own(*new InspectTool);
}

void InspectTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    if (!mouse_is_over_ui && mouseButtonJustPressed(MouseButton::Left)) {
        if (world.get<MapData>().bounds.contains(mouse_tile_pos)) {
            // FIXME: Pass this into the InspectionWindow constructor, or whatever, once that's a thing.
            InspectTool::inspected_tile_pos = mouse_tile_pos;

            V2I windowPos = v2i(the_renderer().ui_camera().mouse_position()) + v2i(16, 16);
            UI::showWindow(UI::WindowTitle::from_lambda([] {
                V2I tilePos = InspectTool::inspected_tile_pos;
                return getText("title_inspect"_s, { formatInt(tilePos.x), formatInt(tilePos.y) });
            }),
                250, 200, windowPos, "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Unique | WindowFlags::UniqueKeepPosition, window_proc, &world);
        }
    }
}

void InspectTool::window_proc(UI::WindowContext* context, void* userData)
{
    DEBUG_FUNCTION();

    auto& world = *static_cast<flecs::world*>(userData);
    auto const& tile_pos = InspectTool::inspected_tile_pos;
    if (!world.get<MapData>().bounds.contains(tile_pos)) {
        context->closeRequested = true;
        return;
    }

    UI::Panel* ui = &context->windowPanel;
    ui->alignWidgets(HAlign::Fill);

    // Terrain
    auto& terrain = world.get<TerrainData>();
    auto terrain_type = terrain.tile_terrain_type.get(tile_pos.x, tile_pos.y);
    auto& terrain_def = TerrainCatalogue::the().get_def(terrain_type);
    ui->addLabel(myprintf("Terrain: {0}, {1} tiles from water"_s, { getText(terrain_def.textAssetName), formatInt(terrain.tile_distance_to_water.get(tile_pos.x, tile_pos.y)) }));

    // Zone
    // ZoneType zone = world->zoneLayer.get_zone_at(tile_pos.x, tile_pos.y);
    // ui->addLabel(myprintf("Zone: {0}"_s, { zone == ZoneType::None ? "None"_s : getText(ZONE_DEFS[zone].textAssetName) }));

    // Building
    auto maybe_building = world.get<BuildingAtPosition>().tile_building.get(tile_pos);
    if (maybe_building.has_value()) {
        auto building = maybe_building.release_value();
        auto& building_component = building.get<BuildingComponent>();
        ui->addLabel(myprintf("Building: {} (Type #{}, Entity #{})"_s, { getText(building.get<Name>().text_asset_name), formatInt(building_component.type), formatInt(building.id()) }));
        ui->addLabel(myprintf("Constructed: {0}"_s, { formatDateTime(dateTimeFromTimestamp(building_component.creation_date), DateTimeFormat::ShortDate) }));
        ui->addLabel(myprintf("Variant: {0}"_s, { building_component.variant_index.map<String>([](auto const& it) { return formatInt(it); }).value_or("None"_s) }));
        // ui->addLabel(myprintf("- Residents: {0} / {1}"_s, { formatInt(building.currentResidents), formatInt(def.residents) }));
        // ui->addLabel(myprintf("- Jobs: {0} / {1}"_s, { formatInt(building.currentJobs), formatInt(def.jobs) }));
        // ui->addLabel(myprintf("- Power: {0}"_s, { formatInt(def.power) }));

        // Problems
        // for (auto problem_type : enum_values<BuildingProblem::Type>()) {
        //     if (building->has_problem(problem_type)) {
        //         ui->addLabel(myprintf("- PROBLEM: {0}"_s, { getText(buildingProblemNames[problem_type]) }));
        //     }
        // }
    } else {
        ui->addLabel("Building: None"_s);
    }

    // Land value
    // ui->addLabel(myprintf("Land value: {0}%"_s, { formatFloat(world->landValueLayer.get_land_value_percent_at(tile_pos.x, tile_pos.y) * 100.0f, 0) }));

    // Debug info
    // auto& inspection_flags = InspectTool::debug_flags;
    // if (inspection_flags.has(InspectTool::DebugFlags::Fire))
    //     world->fireLayer.debug_inspect(*ui, tile_pos, building);
    // if (inspection_flags.has(InspectTool::DebugFlags::Power))
    //     world->powerLayer.debug_inspect(*ui, tile_pos);
    // if (inspection_flags.has(InspectTool::DebugFlags::Transport))
    //     world->transportLayer.debug_inspect(*ui, tile_pos);

    // Highlight
    // Part of me wants this to happen outside of this windowproc, but we don't have a way of knowing when
    // the uiwindow is closed. Maybe at some point we'll want that functionality for other reasons, but
    // for now, it's cleaner and simpler to just do that drawing here.
    // Though, that does mean we can't control *when* the highlight is drawn, or make the building be drawn
    // as highlighted, so maybe this won't work and I'll have to delete this comment in 30 seconds' time!
    // - Sam, 28/08/2019

    auto tileHighlightColor = Colour::from_rgb_255(196, 196, 255, 64);
    auto& renderer = the_renderer();
    drawSingleRect(&renderer.world_overlay_buffer(), Rect2 { tile_pos.x, tile_pos.y, 1, 1 }, renderer.shaderIds.untextured, tileHighlightColor);
}

OwnedRef<BuildTool> BuildTool::create(BuildingType type)
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

void BuildTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    // auto& renderer = the_renderer();
    // auto ghostColorValid = Colour::from_rgb_255(128, 255, 128, 255);
    // auto ghostColorInvalid = Colour::from_rgb_255(255, 0, 0, 128);
    //
    // BuildingDef* buildingDef = getBuildingDef(m_building_type);
    //
    // switch (buildingDef->buildMethod) {
    // case BuildMethod::Paint: // Fallthrough
    // case BuildMethod::Plop: {
    //     if (!mouse_is_over_ui) {
    //         Rect2I footprint = Rect2I::create_centre_size(mouse_tile_pos, buildingDef->size);
    //         s32 buildCost = buildingDef->buildCost;
    //
    //         bool canPlace = world.can_place_building(buildingDef, footprint.x(), footprint.y());
    //
    //         if ((buildingDef->buildMethod == BuildMethod::Plop && mouseButtonJustReleased(MouseButton::Left))
    //             || (buildingDef->buildMethod == BuildMethod::Paint && mouseButtonPressed(MouseButton::Left))) {
    //             if (canPlace && world.can_afford(buildCost)) {
    //                 world.place_building(buildingDef, footprint.x(), footprint.y());
    //                 world.spend(buildCost);
    //             }
    //         }
    //
    //         showCostTooltip(buildCost);
    //
    //         auto& sprite = Sprite::get(buildingDef->spriteName);
    //         auto color = canPlace ? ghostColorValid : ghostColorInvalid;
    //         drawSingleSprite(&renderer.world_overlay_buffer(), &sprite, footprint, renderer.shaderIds.pixelArt, color);
    //     }
    // } break;
    //
    // case BuildMethod::DragLine: // Fallthrough
    // case BuildMethod::DragRect: {
    //     auto [drag_operation, drag_rect] = m_drag_state.update(world.bounds, mouse_tile_pos, mouse_is_over_ui);
    //     s32 buildCost = world.calculate_build_cost(buildingDef, drag_rect);
    //
    //     switch (drag_operation) {
    //     case DragResultOperation::DoAction: {
    //         if (world.can_afford(buildCost)) {
    //             world.place_building_rect(buildingDef, drag_rect);
    //             world.spend(buildCost);
    //         } else {
    //             UI::Toast::show(getText("msg_cannot_afford_construction"_s));
    //         }
    //     } break;
    //
    //     case DragResultOperation::ShowPreview: {
    //         if (!mouse_is_over_ui)
    //             showCostTooltip(buildCost);
    //
    //         if (world.can_afford(buildCost)) {
    //             auto& sprite = Sprite::get(buildingDef->spriteName);
    //             s32 maxGhosts = (drag_rect.width() / buildingDef->size.x) * (drag_rect.height() / buildingDef->size.y);
    //             // TODO: If maxGhosts is 1, just draw 1!
    //             DrawRectsGroup* rectsGroup = beginRectsGroupTextured(&renderer.world_overlay_buffer(), sprite.texture, renderer.shaderIds.pixelArt, maxGhosts);
    //             for (s32 y = 0; y + buildingDef->size.y <= drag_rect.height(); y += buildingDef->size.y) {
    //                 for (s32 x = 0; x + buildingDef->size.x <= drag_rect.width(); x += buildingDef->size.x) {
    //                     bool canPlace = world.can_place_building(buildingDef, drag_rect.x() + x, drag_rect.y() + y);
    //
    //                     Rect2 rect { drag_rect.x() + x, drag_rect.y() + y, buildingDef->size.x, buildingDef->size.y };
    //
    //                     auto color = canPlace ? ghostColorValid : ghostColorInvalid;
    //                     // TODO: All the sprites are the same, so we could optimise this!
    //                     // Then again, eventually we might want ghosts to not be identical, eg ghost roads that visually connect.
    //                     addSpriteRect(rectsGroup, &sprite, rect, color);
    //                 }
    //             }
    //             endRectsGroup(rectsGroup);
    //         } else {
    //             drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(255, 64, 64, 128));
    //         }
    //     } break;
    //
    //     default:
    //         break;
    //     }
    // } break;
    //
    //     INVALID_DEFAULT_CASE;
    // }
}

OwnedRef<DemolishTool> DemolishTool::create()
{
    return adopt_own(*new DemolishTool);
}

void DemolishTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    auto& renderer = the_renderer();
    auto [drag_operation, drag_rect] = m_drag_state.update(world.get<MapData>().bounds, mouse_tile_pos, mouse_is_over_ui);
    auto& budget = world.get_mut<Budget>();

    s32 demolish_cost = 0;
    // FIXME: Figure out how to cache the query for this and the destruction below.
    world.each([&drag_rect, &demolish_cost](Demolishable const& demolishable, BuildingComponent const& building) {
        if (!drag_rect.overlaps(building.footprint))
            return;
        demolish_cost += demolishable.cost;
    });

    switch (drag_operation) {
    case DragResultOperation::DoAction: {
        if (budget.can_afford(demolish_cost)) {
            world.defer([&world, &drag_rect] {
                world.each([&drag_rect](flecs::entity entity, Demolishable const&, BuildingComponent const& building) {
                    if (!drag_rect.overlaps(building.footprint))
                        return;
                    entity.destruct();
                });
            });
            budget.spend(demolish_cost);
        } else {
            UI::Toast::show(getText("msg_cannot_afford_demolition"_s));
        }
    } break;

    case DragResultOperation::ShowPreview: {
        if (!mouse_is_over_ui)
            budget.show_cost_tooltip(demolish_cost);

        if (budget.can_afford(demolish_cost)) {
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

OwnedRef<ZoneTool> ZoneTool::create(ZoneType type)
{
    return adopt_own(*new ZoneTool(type));
}

ZoneTool::ZoneTool(ZoneType type)
    : m_zone_type(type)
{
}

void ZoneTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    // auto& renderer = the_renderer();
    // auto [drag_operation, drag_rect] = m_drag_state.update(world.bounds, mouse_tile_pos, mouse_is_over_ui);
    //
    // CanZoneQuery canZoneQuery = queryCanZoneTiles(&world, m_zone_type, drag_rect);
    // s32 zoneCost = canZoneQuery.calculate_zone_cost();
    //
    // switch (drag_operation) {
    // case DragResultOperation::DoAction: {
    //     if (world.can_afford(zoneCost)) {
    //         placeZone(&world, m_zone_type, drag_rect);
    //         world.spend(zoneCost);
    //     }
    // } break;
    //
    // case DragResultOperation::ShowPreview: {
    //     if (!mouse_is_over_ui)
    //         showCostTooltip(zoneCost);
    //     if (world.can_afford(zoneCost)) {
    //         Colour palette[] = {
    //             Colour::from_rgb_255(255, 0, 0, 16),
    //             ZONE_DEFS[m_zone_type].color
    //         };
    //         drawGrid(&renderer.world_overlay_buffer(), canZoneQuery.bounds, canZoneQuery.tileCanBeZoned, ReadonlySpan { 2, palette });
    //     } else {
    //         drawSingleRect(&renderer.world_overlay_buffer(), drag_rect, renderer.shaderIds.untextured, Colour::from_rgb_255(255, 64, 64, 128));
    //     }
    // } break;
    //
    // default:
    //     break;
    // }
}

OwnedRef<SetTerrainTool> SetTerrainTool::create(TerrainType type)
{
    return adopt_own(*new SetTerrainTool(type));
}

SetTerrainTool::SetTerrainTool(TerrainType type)
    : m_terrain_type(type)
{
}

void SetTerrainTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    // Temporary click-and-drag, no-cost terrain editing
    // We probably want to make this better in several ways, and add a cost to it, and such
    if (!mouse_is_over_ui
        && mouseButtonPressed(MouseButton::Left)
        && world.get<MapData>().bounds.contains(mouse_tile_pos)) {

        // FIXME: What exactly do we want to do when we modify terrain?
        world.get_mut<TerrainData>().tile_terrain_type.set(mouse_tile_pos.x, mouse_tile_pos.y, m_terrain_type);
        world.modified<TerrainData>();
    }
}

OwnedRef<DebugTool> DebugTool::create(Mode mode)
{
    return adopt_own(*new DebugTool(mode));
}

DebugTool::DebugTool(Mode mode)
    : m_mode(mode)
{
}

void DebugTool::act(flecs::world& world, bool mouse_is_over_ui, V2I mouse_tile_pos)
{
    // switch (m_mode) {
    // case Mode::AddFire: {
    //     if (!mouse_is_over_ui
    //         && mouseButtonJustPressed(MouseButton::Left)
    //         && world.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {
    //
    //         world.fireLayer.start_fire_at(world, mouse_tile_pos.x, mouse_tile_pos.y);
    //     }
    //     break;
    // }
    // case Mode::RemoveFire: {
    //     if (!mouse_is_over_ui
    //         && mouseButtonJustPressed(MouseButton::Left)
    //         && world.tile_exists(mouse_tile_pos.x, mouse_tile_pos.y)) {
    //
    //         world.fireLayer.remove_fire_at(world, mouse_tile_pos.x, mouse_tile_pos.y);
    //     }
    //     break;
    // }
    // }
}
