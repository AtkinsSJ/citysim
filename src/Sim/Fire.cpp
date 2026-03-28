/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Fire.h"
#include <App/App.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>
#include <Sim/Effect.h>
#include <UI/Panel.h>
#include <Util/Random.h>

FireLayer::FireLayer(City& city, MemoryArena& arena)
    : m_dirty_rects(arena, city.bounds)
{
    m_funding_level = 1.0f;

    m_tile_fire_proximity_effect = arena.allocate_array_2d<u16>(city.bounds.size());
    m_tile_fire_proximity_effect.fill(0);

    m_tile_total_fire_risk = arena.allocate_array_2d<u8>(city.bounds.size());
    m_tile_total_fire_risk.fill(0);
    m_tile_fire_protection = arena.allocate_array_2d<u8>(city.bounds.size());
    m_tile_fire_protection.fill(0);
    m_tile_overall_fire_risk = arena.allocate_array_2d<u8>(city.bounds.size());
    m_tile_overall_fire_risk.fill(0);

    initChunkedArray(&m_fire_protection_buildings, &city.buildingRefsChunkPool);

    m_active_fire_count = 0;
    initChunkPool(&m_fire_pool, &arena, 64);
    m_sectors = SectorGrid<FireSector> { &arena, city.bounds.size(), 16, 8 };
    for (s32 sectorIndex = 0; sectorIndex < m_sectors.sector_count(); sectorIndex++) {
        FireSector* sector = m_sectors.get_by_index(sectorIndex);

        initChunkedArray(&sector->activeFires, &m_fire_pool);
    }
}

void FireLayer::mark_dirty(Rect2I bounds)
{
    m_dirty_rects.mark_dirty(bounds.expanded(m_max_fire_radius));
}

void FireLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        DEBUG_BLOCK_T("updateFireLayer: building effects", DebugCodeDataTag::Simulation);

        // Recalculate fire distances
        for (auto rectIt = m_dirty_rects.rects().iterate();
            rectIt.hasNext();
            rectIt.next()) {
            Rect2I dirtyRect = rectIt.getValue();
            m_tile_fire_proximity_effect.fill_region(dirtyRect, 0);

            Rect2I expandedRect = dirtyRect.expanded(m_max_fire_radius);
            Rect2I affectedSectors = m_sectors.get_sectors_covered(expandedRect);

            for (s32 sy = affectedSectors.y(); sy < affectedSectors.y() + affectedSectors.height(); sy++) {
                for (s32 sx = affectedSectors.x(); sx < affectedSectors.x() + affectedSectors.width(); sx++) {
                    FireSector* sector = m_sectors.get(sx, sy);

                    for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
                        auto& fire = it.get();
                        if (expandedRect.contains(fire.pos)) {
                            // TODO: Different "strengths" of fire should have different effects
                            EffectRadius fireEffect { m_max_fire_radius, 255, 20 };
                            fireEffect.apply(m_tile_fire_proximity_effect, dirtyRect, v2(fire.pos), EffectType::Add);
                        }
                    }
                }
            }
        }

        m_dirty_rects.clear();
    }

    {
        DEBUG_BLOCK_T("updateFireLayer: overall calculation", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < m_sectors.sectors_to_update_per_tick(); i++) {
            auto [_, sector] = m_sectors.get_next_sector();

            {
                DEBUG_BLOCK_T("updateFireLayer: building fire protection", DebugCodeDataTag::Simulation);
                // Building fire protection
                m_tile_fire_protection.fill_region(sector.bounds, 0);
                for (auto it = m_fire_protection_buildings.iterate(); it.hasNext(); it.next()) {
                    Building* building = city.get_building(it.getValue());
                    if (building != nullptr) {
                        auto& def = building->get_def();

                        float effectiveness = m_funding_level;

                        if (!building->has_power()) {
                            effectiveness *= 0.4f; // @Balance
                        }

                        def.fireProtection.apply(m_tile_fire_protection, sector.bounds, building->footprint.centre(), EffectType::Max, effectiveness);
                    }
                }
            }

            for (s32 y = sector.bounds.y(); y < sector.bounds.y() + sector.bounds.height(); y++) {
                for (s32 x = sector.bounds.x(); x < sector.bounds.x() + sector.bounds.width(); x++) {
                    float tileFireRisk = 0.0f;

                    if (auto* building = city.get_building_at(x, y)) {
                        auto& def = building->get_def();
                        tileFireRisk += def.fireRisk;
                    }

                    // Being near a fire has a HUGE risk!
                    // TODO: Balance this! Currently it's a multiplier on the base building risk,
                    // because if we just ADD a value, then we get fire risk on empty tiles.
                    // But, the balance is definitely off.
                    u16 fireProximityEffect = m_tile_fire_proximity_effect.get(x, y);
                    if (fireProximityEffect > 0) {
                        tileFireRisk += (tileFireRisk * 4.0f * clamp01(fireProximityEffect / 255.0f));
                    }

                    u8 totalRisk = clamp01AndMap_u8(tileFireRisk);
                    m_tile_total_fire_risk.set(x, y, totalRisk);

                    // TODO: Balance this! It feels over-powered, even at 50%.
                    // Possibly fire stations shouldn't affect risk from active fires?
                    float protectionPercent = m_tile_fire_protection.get(x, y) * 0.01f;
                    u8 result = lerp<u8>(totalRisk, 0, protectionPercent * 0.5f);
                    m_tile_overall_fire_risk.set(x, y, result);
                }
            }
        }
    }
}

Optional<Indexed<Fire>> FireLayer::find_fire_at(s32 x, s32 y)
{
    // Optimization: Skip looking for fires if we know there are none.
    if (m_active_fire_count == 0)
        return {};

    auto* sector = m_sectors.get_sector_at_tile_pos(x, y);
    return sector->activeFires.find_first([=](Fire& fire) { return fire.pos.x == x && fire.pos.y == y; });
}

bool FireLayer::does_area_contain_fire(Rect2I bounds) const
{
    bool foundFire = false;

    Rect2I footprintSectors = m_sectors.get_sectors_covered(bounds);
    for (s32 sy = footprintSectors.y();
        sy < footprintSectors.y() + footprintSectors.height() && !foundFire;
        sy++) {
        for (s32 sx = footprintSectors.x();
            sx < footprintSectors.x() + footprintSectors.width() && !foundFire;
            sx++) {
            auto* sector = m_sectors.get(sx, sy);
            for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
                auto& fire = it.get();

                if (bounds.contains(fire.pos)) {
                    foundFire = true;
                    break;
                }
            }
        }
    }

    return foundFire;
}

void FireLayer::start_fire_at(City& city, s32 x, s32 y)
{
    if (auto existing_fire = find_fire_at(x, y); existing_fire.has_value()) {
        // Fire already exists!
        // TODO: make the fire stronger?
        logWarn("Fire at {0},{1} already exists!"_s, { formatInt(x), formatInt(y) });
    } else {
        add_fire_raw(city, x, y, city.gameClock.current_day());
    }
}

void FireLayer::add_fire_raw(City& city, s32 x, s32 y, GameTimestamp start_date)
{
    // NB: We don't care if the fire already exists. startFireAt() already checks for this case.

    FireSector* sector = m_sectors.get_sector_at_tile_pos(x, y);

    Fire* fire = sector->activeFires.appendBlank();

    fire->pos = v2i(x, y);
    fire->startDate = start_date;
    fire->entity = city.add_entity(Entity::Type::Fire, fire);
    // TODO: Probably most of this wants to be moved into addEntity()
    fire->entity->bounds = { x, y, 1, 1 };
    fire->entity->sprite = SpriteRef { "e_fire_1x1"_s, App::the().cosmetic_random().next() };

    m_active_fire_count++;

    mark_dirty({ x, y, 1, 1 });
}

void FireLayer::remove_fire_at(City& city, s32 x, s32 y)
{
    FireSector* sectorAtPosition = m_sectors.get_sector_at_tile_pos(x, y);
    auto existing_fire = sectorAtPosition->activeFires.find_first([=](Fire& fire) { return fire.pos.x == x && fire.pos.y == y; });

    if (existing_fire.has_value()) {
        // Remove it!
        auto& [index, fire] = existing_fire.value();
        city.remove_entity(fire.entity);
        sectorAtPosition->activeFires.take_index(index);
        m_active_fire_count--;

        mark_dirty({ x, y, 1, 1 });
    }
}

void FireLayer::notify_new_building(BuildingDef const& def, Building& building)
{
    if (def.fireProtection.has_effect()) {
        m_fire_protection_buildings.append(building.get_reference());
    }
}

void FireLayer::notify_building_demolished(BuildingDef const& def, Building& building)
{
    if (def.fireProtection.has_effect()) {
        bool success = m_fire_protection_buildings.findAndRemove(building.get_reference());
        ASSERT(success);
    }
}

u8 FireLayer::get_fire_risk_at(s32 x, s32 y) const
{
    return m_tile_total_fire_risk.get(x, y);
}

float FireLayer::get_fire_protection_percent_at(s32 x, s32 y) const
{
    return m_tile_fire_protection.get(x, y) * 0.01f;
}

void FireLayer::debug_inspect(UI::Panel& panel, V2I tile_position, Building* building)
{
    panel.addLabel("*** FIRE INFO ***"_s);

    panel.addLabel(myprintf("There are {0} fire protection buildings and {1} active fires in the city."_s, { formatInt(m_fire_protection_buildings.count), formatInt(m_active_fire_count) }));

    float buildingFireRisk = 100.0f * ((building == nullptr) ? 0.0f : building->get_def().fireRisk);

    panel.addLabel(myprintf("Fire risk: {0}, from:\n- Building: {1}%\n- Nearby fires: {2}"_s,
        {
            formatInt(get_fire_risk_at(tile_position.x, tile_position.y)),
            formatFloat(buildingFireRisk, 1),
            formatInt(m_tile_fire_proximity_effect.get(tile_position.x, tile_position.y)),
        }));

    panel.addLabel(myprintf("Fire protection: {0}%"_s, { formatFloat(get_fire_protection_percent_at(tile_position.x, tile_position.y) * 100.0f, 0) }));

    panel.addLabel(myprintf("Resulting chance of fire: {0}%"_s, { formatFloat(m_tile_overall_fire_risk.get(tile_position.x, tile_position.y) / 2.55f, 1) }));
}

void FireLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Fire>(SAV_FIRE_ID, SAV_FIRE_VERSION);
    SAVSection_Fire fireSection = {};

    // Active fires
    fireSection.activeFireCount = m_active_fire_count;
    Array<SAVFire> tempFires = writer.arena->allocate_array<SAVFire>(fireSection.activeFireCount);
    // ughhhh I have to iterate the sectors to get this information!
    for (s32 sectorIndex = 0; sectorIndex < m_sectors.sector_count(); sectorIndex++) {
        auto* sector = m_sectors.get_by_index(sectorIndex);

        for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
            auto& fire = it.get();
            SAVFire* savFire = tempFires.append();
            *savFire = {};
            savFire->x = (u16)fire.pos.x;
            savFire->y = (u16)fire.pos.y;
            savFire->startDate = fire.startDate;
        }
    }
    fireSection.activeFires = writer.appendArray<SAVFire>(tempFires);

    writer.endSection<SAVSection_Fire>(&fireSection);
}

bool FireLayer::load(BinaryFileReader& reader, City& city)
{
    bool succeeded = false;
    while (reader.startSection(SAV_FIRE_ID, SAV_FIRE_VERSION)) {
        SAVSection_Fire* section = reader.readStruct<SAVSection_Fire>(0);
        if (!section)
            break;

        // Active fires
        Array<SAVFire> tempFires = reader.arena->allocate_array<SAVFire>(section->activeFireCount);
        if (!reader.readArray(section->activeFires, &tempFires))
            break;
        for (u32 activeFireIndex = 0;
            activeFireIndex < section->activeFireCount;
            activeFireIndex++) {
            SAVFire* savFire = &tempFires[activeFireIndex];

            add_fire_raw(city, savFire->x, savFire->y, savFire->startDate);
        }
        ASSERT((u32)m_active_fire_count == section->activeFireCount);

        succeeded = true;
        break;
    }

    return succeeded;
}
