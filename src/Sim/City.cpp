/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "City.h"
#include <App/App.h>
#include <Gfx/Renderer.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <IO/WriteBuffer.h>
#include <Menus/SaveFile.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/Layer.h>
#include <Sim/TerrainCatalogue.h>
#include <Util/Random.h>
#include <Util/Rectangle.h>

NonnullOwnPtr<City> City::create(MemoryArena& arena, u32 width, u32 height, String name, String player_name, s32 funds, GameTimestamp date, float time_of_day)
{
    return adopt_own(*new City(arena, width, height, name, player_name, funds, date, time_of_day));
}

City::City(MemoryArena& arena, u32 width, u32 height, String name, String player_name, s32 funds, GameTimestamp date, float time_of_day)
    : name(arena.allocate_string(name))
    , playerName(arena.allocate_string(player_name))
    , gameClock(date, time_of_day)
    , random(Random::create())
    , funds(funds)
    , bounds(0u, 0u, width, height)
    , tileBuildingIndex(arena.allocate_array_2d<s32>(width, height))
    , buildings(arena, 1024)
    , sectors(&arena, bounds.size(), 16, 8)
    , entities(arena, 1024)
    , sectorBuildingsChunkPool(arena, 128)
    , sectorBoundariesChunkPool(arena, 8)
    , buildingRefsChunkPool(arena, 128)
{

    for (s32 sectorIndex = 0; sectorIndex < sectors.sector_count(); sectorIndex++) {
        CitySector* sector = sectors.get_by_index(sectorIndex);
        new (&sector->ownedBuildings) ChunkedArray { sectorBuildingsChunkPool };
    }

    (void)buildings.append(); // Null building

    // FIXME: Layers need to be initialized after chunk pools etc.
    new (&crimeLayer) CrimeLayer { *this, arena };
    new (&educationLayer) EducationLayer { *this, arena };
    new (&fireLayer) FireLayer { *this, arena };
    new (&healthLayer) HealthLayer { *this, arena };
    new (&landValueLayer) LandValueLayer { *this, arena };
    new (&pollutionLayer) PollutionLayer { *this, arena };
    new (&powerLayer) PowerLayer { *this, arena };
    new (&terrainLayer) TerrainLayer { *this, arena };
    new (&transportLayer) TransportLayer { *this, arena };
    new (&zoneLayer) ZoneLayer { *this, arena };

    m_layers = arena.allocate_array<Layer*>(8);
    m_layers.append(&crimeLayer);
    m_layers.append(&educationLayer);
    m_layers.append(&fireLayer);
    m_layers.append(&healthLayer);
    m_layers.append(&landValueLayer);
    m_layers.append(&pollutionLayer);
    m_layers.append(&powerLayer);
    m_layers.append(&transportLayer);

    // TODO: The rest of this code doesn't really belong here!
    // It belongs in a "we've just started/loaded a game, so initialise things" place.

    // TODO: Are we sure we want to do this?
    mark_area_dirty(bounds);

    the_renderer().world_camera().set_position(v2(bounds.size()) / 2);

    saveBuildingTypes();
    saveTerrainTypes();
}

void City::remove_entity(Entity* entity)
{
    // logInfo("Removing entity #{0}"_s, {formatInt(entity->index)});
    entities.removeIndex(entity->index);
}

void City::draw_entities(Rect2I visibleTileBounds) const
{
    // TODO: Depth sorting
    // TODO: Sectors maybe? Though collecting all the visible entities into a data structure might be slower than just
    //  iterating the entities array.
    Rect2 cropArea = visibleTileBounds;
    auto shaderID = the_renderer().shaderIds.pixelArt;

    bool isDemolitionHappening = demolitionRect.has_positive_area();
    auto drawColorDemolish = Colour::from_rgb_255(255, 128, 128, 255);

    for (auto it = entities.iterate(); it.hasNext(); it.next()) {
        auto entity = it.get();
        if (cropArea.overlaps(entity->bounds)) {
            // TODO: Batch these together somehow? Our batching is a bit complicated.
            // OK, turns out our renderer still batches same-texture-same-shader calls into a single draw call!
            // So, the difference is just sending N x RenderItem_DrawSingleRect instead of 1 x RenderItem_DrawRects
            // Thanks, past me!
            // - Sam, 26/09/2020

            auto drawColor = entity->color;

            if (entity->canBeDemolished && isDemolitionHappening && entity->bounds.overlaps(demolitionRect)) {
                drawColor = drawColor.multiplied_by(drawColorDemolish);
            }

            drawSingleSprite(&the_renderer().world_buffer(), &entity->sprite.get(), entity->bounds, shaderID, drawColor);
        }
    }
}

Building* City::add_building_direct(s32 id, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate)
{
    DEBUG_FUNCTION();

    // FIXME: This is weird, we should construct in one go.
    auto [building_index, building] = buildings.empend(id, *def, footprint, creationDate);

    // Random sprite!
    building.spriteOffset = App::the().cosmetic_random().random_integer<u16>();

    building.entity = add_entity(Entity::Type::Building, &building);
    building.entity->bounds = footprint;
    building.load_sprite();
    building.entity->canBeDemolished = true;

    CitySector* ownerSector = sectors.get_sector_at_tile_pos(footprint.x(), footprint.y());

    ownerSector->ownedBuildings.append(&building);

    for (s32 y = footprint.y();
        y < footprint.y() + footprint.height();
        y++) {
        for (s32 x = footprint.x();
            x < footprint.x() + footprint.width();
            x++) {
            tileBuildingIndex.set(x, y, building_index);
        }
    }

    for (auto const& layer : m_layers)
        layer->notify_new_building(*def, building);

    return &building;
}

Building* City::add_building(BuildingDef* def, Rect2I footprint, Optional<GameTimestamp> const& creation_date)
{
    DEBUG_FUNCTION();

    Building* building = add_building_direct(++highestBuildingID, def, footprint, creation_date.value_or(gameClock.current_day()));

    // TODO: Properly calculate occupancy!
    building->currentResidents = def->residents;
    building->currentJobs = def->jobs;

    return building;
}

void City::mark_area_dirty(Rect2I dirty_area)
{
    for (auto& layer : m_layers)
        layer->mark_dirty(dirty_area);
}

bool City::tile_exists(s32 x, s32 y) const
{
    return (x >= 0) && (x < bounds.width())
        && (y >= 0) && (y < bounds.height());
}

bool City::can_afford(s32 cost) const
{
    return funds >= cost;
}

void City::spend(s32 cost)
{
    funds -= cost;
}

bool City::can_place_building(BuildingDef* def, s32 left, s32 top) const
{
    DEBUG_FUNCTION();

    // Can we afford to build this?
    if (!can_afford(def->buildCost)) {
        return false;
    }

    Rect2I footprint { { left, top }, def->size };

    // Are we in bounds?
    if (!bounds.contains(footprint)) {
        return false;
    }

    auto& catalogue = BuildingCatalogue::the();

    // Check terrain is buildable and empty
    // TODO: Optimise this per-sector!
    for (s32 y = footprint.y(); y < footprint.y() + footprint.height(); y++) {
        for (s32 x = footprint.x(); x < footprint.x() + footprint.width(); x++) {
            auto& terrain_def = terrainLayer.terrain_at(x, y);

            if (!terrain_def.canBuildOn)
                return false;

            auto* buildingAtPos = get_building_at(x, y);
            if (buildingAtPos != nullptr) {
                // Check if we can combine this with the building that's already there
                if (catalogue.find_building_intersection(buildingAtPos->get_def(), *def).has_value()) {
                    // We can!
                    // TODO: We want to check if there is a valid variant, before we build.
                    // But that means matching against buildings that aren't constructed yet,
                    // but just part of the drag-rect planned construction!
                    // (eg, dragging a road across a rail, we want to check for a rail-crossing
                    // variant that matches the planned road tiles.)
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}

void City::place_building(BuildingDef* def, s32 left, s32 top, bool markAreasDirty)
{
    DEBUG_FUNCTION();

    Rect2I footprint { { left, top }, def->size };

    Building* building = get_building_at(left, top);
    if (building != nullptr) {
        // Do a quick replace! We already established in canPlaceBuilding() that we match.
        // NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
        auto& old_def = building->get_def();
        auto& intersection_def = BuildingCatalogue::the().find_building_intersection(old_def, *def).release_value();

        building->typeID = intersection_def.typeID;
        def = const_cast<BuildingDef*>(&intersection_def); // I really don't like this but I don't want to rewrite this entire function right now!

        zoneLayer.population[old_def.growsInZone] -= building->currentResidents + building->currentJobs;
    } else {
        // Remove zones
        placeZone(this, ZoneType::None, footprint);

        building = add_building(def, footprint);
    }

    // TODO: Calculate residents/jobs properly!
    building->currentResidents = def->residents;
    building->currentJobs = def->jobs;

    zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;

    building->update_variant(*this, *def);
    update_adjacent_building_variants(footprint);

    if (markAreasDirty) {
        mark_area_dirty(footprint);
    }
}

s32 City::calculate_build_cost(BuildingDef* def, Rect2I area) const
{
    DEBUG_FUNCTION();

    s32 totalCost = 0;

    for (s32 y = 0; y + def->size.y <= area.height(); y += def->size.y) {
        for (s32 x = 0; x + def->size.x <= area.width(); x += def->size.x) {
            if (can_place_building(def, area.x() + x, area.y() + y)) {
                totalCost += def->buildCost;
            }
        }
    }

    return totalCost;
}

void City::place_building_rect(BuildingDef* def, Rect2I area)
{
    DEBUG_FUNCTION();

    for (s32 y = 0; y + def->size.y <= area.height(); y += def->size.y) {
        for (s32 x = 0; x + def->size.x <= area.width(); x += def->size.x) {
            if (can_place_building(def, area.x() + x, area.y() + y)) {
                place_building(def, area.x() + x, area.y() + y, false);
            }
        }
    }

    mark_area_dirty(area);
}

s32 City::calculate_demolition_cost(Rect2I area) const
{
    DEBUG_FUNCTION();

    s32 total = 0;

    // Building demolition cost
    for_each_building_overlapping_area(area, {}, [&](auto& building) {
        total += building.get_def().demolishCost;
    });

    return total;
}

void City::demolish_rect(Rect2I area)
{
    DEBUG_FUNCTION();

    // NB: We assume that we've already checked we can afford this!

    // Building demolition
    // FIXME: We first accumulate the buildings into an array to make removal safe. Find some way to avoid this?
    ChunkedArray<Building*> buildingsToDemolish { temp_arena(), 1024 };
    for_each_building_overlapping_area(area, {}, [&](Building& building) {
        buildingsToDemolish.append(&building);
    });
    for (auto it = buildingsToDemolish.iterate(buildingsToDemolish.count - 1, false, true);
        it.hasNext();
        it.next()) {
        Building* building = it.getValue();
        auto& def = building->get_def();

        zoneLayer.population[def.growsInZone] -= building->currentResidents + building->currentJobs;

        Rect2I buildingFootprint = building->footprint;

        // Clean up other references
        for (auto& layer : m_layers)
            layer->notify_building_demolished(def, *building);

        building->id = 0;

        s32 buildingIndex = tileBuildingIndex.get(buildingFootprint.x(), buildingFootprint.y());
        buildings.removeIndex(buildingIndex);
        remove_entity(building->entity);

        building = nullptr; // For safety, because we just deleted the Building!

        for (s32 y = buildingFootprint.y();
            y < buildingFootprint.y() + buildingFootprint.height();
            y++) {
            for (s32 x = buildingFootprint.x();
                x < buildingFootprint.x() + buildingFootprint.width();
                x++) {
                tileBuildingIndex.set(x, y, 0);
            }
        }

        // Only need to add the footprint as a separate rect if it's not inside the area!
        if (!area.contains(buildingFootprint)) {
            mark_area_dirty(buildingFootprint);
        }
    }

    // Expand the area to account for buildings to the left or up from it
    auto& building_catalogue = BuildingCatalogue::the();
    Rect2I expandedArea = area.expanded(building_catalogue.overallMaxBuildingDim, 0, 0, building_catalogue.overallMaxBuildingDim);
    Rect2I sectorsArea = sectors.get_sectors_covered(expandedArea);

    for (s32 sY = sectorsArea.y();
        sY < sectorsArea.y() + sectorsArea.height();
        sY++) {
        for (s32 sX = sectorsArea.x();
            sX < sectorsArea.x() + sectorsArea.width();
            sX++) {
            CitySector* sector = sectors.get(sX, sY);

            // Rebuild the ownedBuildings array
            sector->ownedBuildings.clear();

            for (s32 y = sector->bounds.y();
                y < sector->bounds.y() + sector->bounds.height();
                y++) {
                for (s32 x = sector->bounds.x();
                    x < sector->bounds.x() + sector->bounds.width();
                    x++) {
                    Building* b = get_building_at(x, y);
                    if (b != nullptr) {
                        if (b->footprint.x() == x && b->footprint.y() == y) {
                            sector->ownedBuildings.append(b);
                        }
                    }
                }
            }
        }
    }

    // Mark area as changed
    mark_area_dirty(area);

    // Any buildings that would have connected with something that just got demolished need to refresh!
    update_adjacent_building_variants(area);
}

void City::for_each_building_overlapping_area(Rect2I area, Flags<BuildingQueryFlag> flags, Function<void(Building&)> const& callback)
{
    DEBUG_FUNCTION();

    // Expand the area to account for buildings to the left or up from it
    // (but don't do that if we only care about origins)
    s32 expansion = flags.has(BuildingQueryFlag::RequireOriginInArea) ? 0 : BuildingCatalogue::the().overallMaxBuildingDim;
    Rect2I expandedArea = area.expanded(expansion, 0, 0, expansion);
    Rect2I sectorsArea = sectors.get_sectors_covered(expandedArea);

    for (s32 sY = sectorsArea.y();
        sY < sectorsArea.y() + sectorsArea.height();
        sY++) {
        for (s32 sX = sectorsArea.x();
            sX < sectorsArea.x() + sectorsArea.width();
            sX++) {
            auto* sector = sectors.get(sX, sY);

            for (auto it = sector->ownedBuildings.iterate(); it.hasNext(); it.next()) {
                Building* building = it.getValue();
                if (building->footprint.overlaps(area))
                    callback(*building);
            }
        }
    }
}

void City::for_each_building_overlapping_area(Rect2I area, Flags<BuildingQueryFlag> flags, Function<void(Building const&)> const& callback) const
{
    const_cast<City*>(this)->for_each_building_overlapping_area(area, flags, [&callback](auto const& building) {
        callback(const_cast<Building&>(building));
    });
}

void City::draw(Rect2I visible_tile_bounds) const
{
    auto& renderer = the_renderer();
    terrainLayer.draw_terrain(visible_tile_bounds, renderer.shaderIds.pixelArt);

    zoneLayer.draw_zones(visible_tile_bounds, renderer.shaderIds.untextured);

    draw_entities(visible_tile_bounds);

    // Draw sectors
    // NB: this is really hacky debug code
    if (false) {
        Rect2I visibleSectors = sectors.get_sectors_covered(visible_tile_bounds);
        DrawRectsGroup* group = beginRectsGroupUntextured(&renderer.world_overlay_buffer(), renderer.shaderIds.untextured, visibleSectors.area());
        auto sectorColor = Colour::from_rgb_255(255, 255, 255, 40);
        for (s32 sy = visibleSectors.y();
            sy < visibleSectors.y() + visibleSectors.height();
            sy++) {
            for (s32 sx = visibleSectors.x();
                sx < visibleSectors.x() + visibleSectors.width();
                sx++) {
                if ((sx + sy) % 2) {
                    auto* sector = sectors.get(sx, sy);
                    addUntexturedRect(group, sector->bounds, sectorColor);
                }
            }
        }
        endRectsGroup(group);
    }
}

bool City::building_exists_at(s32 x, s32 y) const
{
    return tileBuildingIndex.get(x, y) > 0;
}

Building* City::get_building_at(s32 x, s32 y)
{
    Building* result = nullptr;

    if (tile_exists(x, y)) {
        u32 buildingID = tileBuildingIndex.get(x, y);
        if (buildingID > 0) {
            result = buildings.get(buildingID);
        }
    }

    return result;
}

void City::update()
{
    zoneLayer.update(*this);

    for (auto& layer : m_layers)
        layer->update(*this);

    // Runs an update on X sectors' buildings, gradually covering the whole city with subsequent calls.
    for (s32 i = 0; i < sectors.sectors_to_update_per_tick(); i++) {
        auto [_, sector] = sectors.get_next_sector();

        for (auto it = sector.ownedBuildings.iterate(); it.hasNext(); it.next()) {
            Building* building = it.getValue();
            building->update(*this);
        }
    }
}

void City::save_buildings(BinaryFileWriter* writer) const
{
    writer->startSection<SAVSection_Buildings>(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
    SAVSection_Buildings buildingSection = {};

    // Building types table
    auto& building_catalogue = BuildingCatalogue::the();
    s32 buildingDefCount = building_catalogue.allBuildings.count - 1; // Skip the null def
    WriteBufferRange buildingTypeTableLoc = writer->reserveArray<SAVBuildingTypeEntry>(buildingDefCount);
    Array<SAVBuildingTypeEntry> buildingTypeTable = writer->arena->allocate_array<SAVBuildingTypeEntry>(buildingDefCount);
    for (auto it = building_catalogue.allBuildings.iterate(); it.hasNext(); it.next()) {
        BuildingDef* def = it.get();
        if (def->typeID == 0)
            continue; // Skip the null building def!

        SAVBuildingTypeEntry* entry = buildingTypeTable.append();
        entry->typeID = def->typeID;
        entry->name = writer->append_string(def->name);
    }
    buildingSection.buildingTypeTable = writer->writeArray<SAVBuildingTypeEntry>(buildingTypeTable, buildingTypeTableLoc);

    // Highest ID
    buildingSection.highestBuildingID = highestBuildingID;

    //
    // The buildings themselves!
    // I'm not sure how to actually do this... current thought is that the "core" building
    // data will be here, and that other stuff (eg, health/education of the occupants)
    // will be in the relevant layers' chunks. That seems the most extensible?
    // Another option would be to store the buildings here as a series of arrays, one
    // for each field, like we do for Terrain. That feels really messy though.
    // The tricky thing is that the Building struct in game feels likely to change a lot,
    // and we want the save format to change as little as possible... though that only
    // matters once the game is released (or near enough release that I start making
    // pre-made maps) so maybe it's not such a big issue?
    // Eh, going to just go ahead with a placeholder version, like the rest of this code!
    //
    // - Sam, 11/10/2019
    //
    buildingSection.buildingCount = buildings.count - 1; // Not the null building!

    auto tempBuildings = writer->arena->allocate_multiple<SAVBuilding>(buildingSection.buildingCount);
    s32 tempBuildingIndex = 0;

    for (auto it = buildings.iterate(); it.hasNext(); it.next()) {
        Building* building = it.get();
        if (building->id == 0)
            continue; // Skip the null building!

        tempBuildings[tempBuildingIndex] = {
            .id = building->id,
            .typeID = building->typeID,
            .creationDate = building->creationDate,
            .x = (u16)building->footprint.x(),
            .y = (u16)building->footprint.y(),
            .w = (u16)building->footprint.width(),
            .h = (u16)building->footprint.height(),
            .spriteOffset = building->spriteOffset,
            .currentResidents = (u16)building->currentResidents,
            .currentJobs = (u16)building->currentJobs,
            .variantIndex = building->variantIndex.value_or(-1),
        };

        tempBuildingIndex++;
    }
    buildingSection.buildings = writer->appendBlob(buildingSection.buildingCount * sizeof(SAVBuilding), (u8*)tempBuildings.raw_data(), FileBlobCompressionScheme::Uncompressed);

    writer->endSection<SAVSection_Buildings>(&buildingSection);
}

bool City::load_buildings(BinaryFileReader* reader)
{
    bool succeeded = reader->startSection(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
    while (succeeded) {
        SAVSection_Buildings* section = reader->readStruct<SAVSection_Buildings>(0);
        if (!section)
            break;

        highestBuildingID = section->highestBuildingID;

        // Map the file's building type IDs to the game's ones
        // NB: count+1 because the file won't save the null building, so we need to compensate
        Array<u32> oldTypeToNewType = reader->arena->allocate_array<u32>(section->buildingTypeTable.count + 1, true);
        Array<SAVBuildingTypeEntry> buildingTypeTable = reader->arena->allocate_array<SAVBuildingTypeEntry>(section->buildingTypeTable.count);
        if (!reader->readArray(section->buildingTypeTable, &buildingTypeTable))
            break;
        for (auto const& entry : buildingTypeTable) {
            String buildingName = reader->readString(entry.name);

            BuildingDef* def = findBuildingDef(buildingName);
            if (def == nullptr) {
                // The building doesn't exist in the game... we'll remap to 0
                //
                // Ideally, we'd keep the information about what a building really is, so
                // that if it's later loaded into a game that does have that building, it'll work
                // again instead of being forever lost. But, we're talking about a situation of
                // the player messing around with the data files, or adding/removing/adding mods,
                // so IDK. The saved game is already corrupted if you load it and stuff is missing -
                // playing at all from that point is going to break things, and if we later make
                // that stuff appear again, we're actually just breaking it a second time!
                //
                // So maybe, the real "correct" solution is to tell the player that things are missing
                // from the game, and then just demolish the missing-id buildings.
                //
                // Mods probably want some way to define "migrations" for transforming saved games
                // when the mod's data changes. That's probably a good idea for the base game's data
                // too, because changes happen!
                //
                // Lots to think about!
                //
                // - Sam, 11/11/2019
                //
                oldTypeToNewType[entry.typeID] = 0;
            } else {
                oldTypeToNewType[entry.typeID] = def->typeID;
            }
        }

        Array<SAVBuilding> tempBuildings = reader->arena->allocate_array<SAVBuilding>(section->buildingCount);
        if (!reader->readBlob(section->buildings, &tempBuildings))
            break;
        for (u32 buildingIndex = 0;
            buildingIndex < section->buildingCount;
            buildingIndex++) {
            SAVBuilding* savBuilding = &tempBuildings[buildingIndex];

            Rect2I footprint { savBuilding->x, savBuilding->y, savBuilding->w, savBuilding->h };
            BuildingDef* def = getBuildingDef(oldTypeToNewType[savBuilding->typeID]);
            Building* building = add_building_direct(savBuilding->id, def, footprint, savBuilding->creationDate);
            building->variantIndex = savBuilding->variantIndex == -1 ? Optional<s16> {} : Optional<s16> { savBuilding->variantIndex.value() };
            building->spriteOffset = savBuilding->spriteOffset;
            building->currentResidents = savBuilding->currentResidents;
            building->currentJobs = savBuilding->currentJobs;
            // Because the sprite was assigned in addBuildingDirect(), before we loaded the variant, we
            // need to overwrite the sprite here.
            // Probably there's a better way to organise this, but this works.
            // - Sam, 26/09/2020
            building->load_sprite();

            // This is a bit hacky but it's how we calculate it elsewhere
            zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;
        }

        break;
    }

    return succeeded;
}

Building* City::get_building(BuildingRef const& ref)
{
    Building* building = get_building_at(ref.position().x, ref.position().y);
    if ((building != nullptr) && (building->id == ref.id())) {
        return building;
    }

    return nullptr;
}

void City::update_adjacent_building_variants(Rect2I footprint)
{
    DEBUG_FUNCTION();

    for (s32 y = footprint.y(); y < footprint.y() + footprint.height(); y++) {
        if (auto* left_building = get_building_at(footprint.x() - 1, y)) {
            if (auto& left_def = left_building->get_def(); left_def.variants.count > 0)
                left_building->update_variant(*this, left_def);
        }

        if (auto* right_building = get_building_at(footprint.x() + footprint.width(), y)) {
            if (auto& right_def = right_building->get_def(); right_def.variants.count > 0)
                right_building->update_variant(*this, right_def);
        }
    }

    for (s32 x = footprint.x(); x < footprint.x() + footprint.width(); x++) {
        if (auto* up_building = get_building_at(x, footprint.y() - 1)) {
            if (auto& up_def = up_building->get_def(); up_def.variants.count > 0)
                up_building->update_variant(*this, up_def);
        }
        if (auto* down_building = get_building_at(x, footprint.y() + footprint.height())) {
            if (auto& down_def = down_building->get_def(); down_def.variants.count > 0)
                down_building->update_variant(*this, down_def);
        }
    }
}
