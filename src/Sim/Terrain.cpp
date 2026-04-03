/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Terrain.h"

#include <App/App.h>
#include <Gfx/Renderer.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/Game.h>
#include <Sim/TerrainCatalogue.h>
#include <Sim/Tool.h>
#include <UI/Window.h>
#include <Util/Splat.h>

TerrainLayer::TerrainLayer(City& city, MemoryArena& arena)
    : m_bounds(city.bounds)
    , m_tile_terrain_type(arena.allocate_array_2d<u8>(m_bounds.size()))
    , m_tile_height(arena.allocate_array_2d<u8>(m_bounds.size()))
    , m_tile_distance_to_water(arena.allocate_array_2d<u8>(m_bounds.size()))
    , m_tile_sprite_offset(arena.allocate_array_2d<u8>(m_bounds.size()))
    , m_tile_sprite(arena.allocate_array_2d<SpriteRef>(m_bounds.size()))
    , m_tile_border_sprite(arena.allocate_array_2d<Optional<SpriteRef>>(m_bounds.size()))
{
}

TerrainDef const& TerrainLayer::terrain_at(s32 x, s32 y) const
{
    return TerrainCatalogue::the().get_def(terrain_type_at(x, y));
}

u8 TerrainLayer::terrain_type_at(s32 x, s32 y) const
{
    return m_tile_terrain_type.get_if_exists(x, y, 0);
}

u8 TerrainLayer::height_at(s32 x, s32 y) const
{
    return m_tile_height.get(x, y);
}

void TerrainLayer::set_terrain_at(s32 x, s32 y, TerrainType type)
{
    u8 existingTerrain = m_tile_terrain_type.get_if_exists(x, y, 0);
    // Ignore for tiles that don't exist, or are already the desired type
    if (existingTerrain == 0 || existingTerrain == type) {
        return;
    }

    // Set the terrain
    m_tile_terrain_type.set(x, y, type);

    // Update sprites on this and neighbouring tiles
    Rect2I sprite_update_bounds = m_bounds.intersected({ x - 1, y - 1, 3, 3 });
    assign_terrain_sprites(sprite_update_bounds);

    // Update distance to water
    update_distance_to_water({ x, y, 1, 1 });
}

u8 TerrainLayer::distance_to_water_at(s32 x, s32 y) const
{
    return m_tile_distance_to_water.get(x, y);
}

void TerrainLayer::update_distance_to_water(Rect2I bounds)
{
    bounds = m_bounds.intersected(bounds.expanded(maxDistanceToWater));
    u8 tWater = truncate<u8>(findTerrainTypeByName("water"_s));

    for (s32 y = bounds.y();
        y < bounds.y() + bounds.height();
        y++) {
        for (s32 x = bounds.x();
            x < bounds.x() + bounds.width();
            x++) {
            u8 tileType = m_tile_terrain_type.get_if_exists(x, y, 0);
            if (tileType == tWater) {
                m_tile_distance_to_water.set(x, y, 0);
            } else {
                m_tile_distance_to_water.set(x, y, 255);
            }
        }
    }

    updateDistances(&m_tile_distance_to_water, bounds, maxDistanceToWater);
}

void TerrainLayer::draw_terrain(Rect2I visible_area, s8 shader_id) const
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    auto& renderer = the_renderer();

    Rect2 spriteBounds { 0.0f, 0.0f, 1.0f, 1.0f };
    auto white = Colour::white();

    for (s32 y = visible_area.y();
        y < visible_area.y() + visible_area.height();
        y++) {
        spriteBounds.set_y(y);

        for (s32 x = visible_area.x();
            x < visible_area.x() + visible_area.width();
            x++) {
            Sprite* sprite = &m_tile_sprite.get(x, y).get();
            spriteBounds.set_x(x);
            drawSingleSprite(&renderer.world_buffer(), sprite, spriteBounds, shader_id, white);

            if (auto& border_sprite_ref = m_tile_border_sprite.get(x, y); border_sprite_ref.has_value()) {
                auto& border_sprite = border_sprite_ref.value().get();
                drawSingleSprite(&renderer.world_buffer(), &border_sprite, spriteBounds, shader_id, white);
            }
        }
    }
}

void TerrainLayer::generate(City& city, u32 seed)
{
    DEBUG_FUNCTION();

    auto& cosmetic_random = App::the().cosmetic_random();

    u8 tGround = truncate<u8>(findTerrainTypeByName("ground"_s));
    u8 tWater = truncate<u8>(findTerrainTypeByName("water"_s));
    BuildingDef* treeDef = findBuildingDef("tree"_s);

    auto terrainRandom = Random::create(seed);
    m_terrain_generation_seed = seed;
    m_tile_terrain_type.fill(tGround);

    for (s32 y = 0; y < m_bounds.height(); y++) {
        for (s32 x = 0; x < m_bounds.width(); x++) {
            m_tile_sprite_offset.set(x, y, cosmetic_random.random_integer<u8>());
        }
    }

    // Generate a river
    Array<float> riverOffset = temp_arena().allocate_filled_array<float>(m_bounds.height());
    terrainRandom->fill_with_noise(riverOffset, 10);
    float riverMaxWidth = terrainRandom->random_float_between(12, 16);
    float riverMinWidth = terrainRandom->random_float_between(6, riverMaxWidth);
    float riverWaviness = 16.0f;
    s32 riverCentreBase = terrainRandom->random_between(ceil_s32(m_bounds.width() * 0.4f), floor_s32(m_bounds.width() * 0.6f));
    for (s32 y = 0; y < m_bounds.height(); y++) {
        s32 riverWidth = ceil_s32(lerp(riverMinWidth, riverMaxWidth, ((float)y / (float)m_bounds.height())));
        s32 riverCentre = riverCentreBase - round_s32((riverWaviness * 0.5f) + (riverWaviness * riverOffset[y]));
        s32 riverLeft = riverCentre - (riverWidth / 2);

        for (s32 x = riverLeft; x < riverLeft + riverWidth; x++) {
            m_tile_terrain_type.set(x, y, tWater);
        }
    }

    // Coastline
    Array<float> coastlineOffset = temp_arena().allocate_filled_array<float>(m_bounds.width());
    terrainRandom->fill_with_noise(coastlineOffset, 10);
    for (s32 x = 0; x < m_bounds.width(); x++) {
        s32 coastDepth = 8 + round_s32(coastlineOffset[x] * 16.0f);

        for (s32 i = 0; i < coastDepth; i++) {
            s32 y = m_bounds.height() - 1 - i;

            m_tile_terrain_type.set(x, y, tWater);
        }
    }

    // Lakes/ponds
    s32 pondCount = terrainRandom->random_between(1, 4);
    for (s32 pondIndex = 0; pondIndex < pondCount; pondIndex++) {
        s32 pondCentreX = terrainRandom->random_between(0, m_bounds.width());
        s32 pondCentreY = terrainRandom->random_between(0, m_bounds.height());

        float pondMinRadius = terrainRandom->random_float_between(3.0f, 5.0f);
        float pondMaxRadius = terrainRandom->random_float_between(pondMinRadius + 3.0f, 20.0f);

        Splat pondSplat = Splat::create_random(pondCentreX, pondCentreY, pondMinRadius, pondMaxRadius, 36, terrainRandom);

        Rect2I boundingBox = pondSplat.bounding_box().intersected(m_bounds);
        for (s32 y = boundingBox.y(); y < boundingBox.y() + boundingBox.height(); y++) {
            for (s32 x = boundingBox.x(); x < boundingBox.x() + boundingBox.width(); x++) {
                if (pondSplat.contains(x, y)) {
                    m_tile_terrain_type.set(x, y, tWater);
                }
            }
        }
    }

    // Forest splats
    if (treeDef == nullptr) {
        logError("Map generator is unable to place any trees, because the 'tree' building was not found."_s);
    } else {
        s32 forestCount = terrainRandom->random_between(10, 20);
        for (s32 forestIndex = 0; forestIndex < forestCount; forestIndex++) {
            s32 centreX = terrainRandom->random_between(0, m_bounds.width());
            s32 centreY = terrainRandom->random_between(0, m_bounds.height());

            float minRadius = terrainRandom->random_float_between(2.0f, 8.0f);
            float maxRadius = terrainRandom->random_float_between(minRadius + 1.0f, 30.0f);

            Splat forestSplat = Splat::create_random(centreX, centreY, minRadius, maxRadius, 36, terrainRandom);

            Rect2I boundingBox = forestSplat.bounding_box().intersected(m_bounds);
            for (s32 y = boundingBox.y(); y < boundingBox.y() + boundingBox.height(); y++) {
                for (s32 x = boundingBox.x(); x < boundingBox.x() + boundingBox.width(); x++) {
                    if (terrain_at(x, y).canBuildOn
                        && (city.get_building_at(x, y) == nullptr)
                        && forestSplat.contains(x, y)) {
                        city.add_building(treeDef, { x, y, treeDef->size.x, treeDef->size.y });
                    }
                }
            }
        }
    }

    assign_terrain_sprites(m_bounds);
    update_distance_to_water(m_bounds);
}

void TerrainLayer::assign_terrain_sprites(Rect2I bounds)
{
    // Assign a terrain tile variant for each tile, depending on its neighbours
    for (s32 y = bounds.y();
        y < bounds.y() + bounds.height();
        y++) {
        for (s32 x = bounds.x();
            x < bounds.x() + bounds.width();
            x++) {
            auto& def = terrain_at(x, y);

            m_tile_sprite.set(x, y, SpriteRef { def.spriteName, m_tile_sprite_offset.get(x, y) });

            if (def.drawBordersOver) {
                // First, determine if we have a bordering type
                TerrainDef const* borders[8] {
                    // Starting NW, going clockwise
                    &terrain_at(x - 1, y - 1),
                    &terrain_at(x, y - 1),
                    &terrain_at(x + 1, y - 1),
                    &terrain_at(x + 1, y),
                    &terrain_at(x + 1, y + 1),
                    &terrain_at(x, y + 1),
                    &terrain_at(x - 1, y + 1),
                    &terrain_at(x - 1, y),
                };
                // Find the first match, if any
                // Eventually we probably want to use whichever border terrain is most common instead
                TerrainDef const* borderTerrain = nullptr;
                for (s32 i = 0; i < 8; i++) {
                    if ((borders[i] != &def) && (!borders[i]->borderSpriteNames.is_empty())) {
                        borderTerrain = borders[i];
                        break;
                    }
                }

                if (borderTerrain) {
                    // Determine which border sprite to use based on our borders
                    u8 n = (borders[1] == borderTerrain) ? 2 : (borders[0] == borderTerrain) ? 1
                                                                                             : 0;
                    u8 e = (borders[3] == borderTerrain) ? 2 : (borders[2] == borderTerrain) ? 1
                                                                                             : 0;
                    u8 s = (borders[5] == borderTerrain) ? 2 : (borders[4] == borderTerrain) ? 1
                                                                                             : 0;
                    u8 w = (borders[7] == borderTerrain) ? 2 : (borders[6] == borderTerrain) ? 1
                                                                                             : 0;

                    u8 borderSpriteIndex = w + (s * 3) + (e * 9) + (n * 27) - 1;
                    ASSERT(borderSpriteIndex >= 0 && borderSpriteIndex <= 80);
                    m_tile_border_sprite.set(x, y, SpriteRef { borderTerrain->borderSpriteNames[borderSpriteIndex], m_tile_sprite_offset.get(x, y) });
                } else {
                    m_tile_border_sprite.set(x, y, {});
                }
            } else {
                m_tile_border_sprite.set(x, y, {});
            }
        }
    }
}

void show_terrain_window()
{
    UI::showWindow(UI::WindowTitle::from_text_asset("title_terrain"_s), 300, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::UniqueKeepPosition | WindowFlags::AutomaticHeight, modify_terrain_window_proc);
}

void modify_terrain_window_proc(UI::WindowContext* context, void*)
{
    auto* game_scene = dynamic_cast<GameScene*>(&App::the().scene());
    if (!game_scene) {
        context->closeRequested = true;
        return;
    }

    UI::Panel* ui = &context->windowPanel;

    auto terrain_tool_is_active = [&](TerrainType type) {
        if (auto* terrain_tool = dynamic_cast<SetTerrainTool const*>(&game_scene->active_tool()); terrain_tool && terrain_tool->terrain_type() == type)
            return ButtonState::Active;
        return ButtonState::Normal;
    };

    for (auto it = TerrainCatalogue::the().terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* terrain = it.get();
        if (terrain->typeID == 0)
            continue; // Skip the null terrain

        if (ui->addImageButton(&Sprite::get(terrain->spriteName), terrain_tool_is_active(terrain->typeID))) {
            game_scene->set_active_tool(SetTerrainTool::create(terrain->typeID));
        }
    }
}

void TerrainLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Terrain>(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION);
    SAVSection_Terrain terrainSection = {};

    // Terrain generation parameters
    terrainSection.terrainGenerationSeed = m_terrain_generation_seed;

    // Terrain types table
    auto& terrain_catalogue = TerrainCatalogue::the();
    s32 terrainDefCount = terrain_catalogue.terrainDefs.count - 1; // Skip the null def
    WriteBufferRange terrainTypeTableLoc = writer.reserveArray<SAVTerrainTypeEntry>(terrainDefCount);
    Array<SAVTerrainTypeEntry> terrainTypeTable = writer.arena->allocate_array<SAVTerrainTypeEntry>(terrainDefCount);
    for (auto it = terrain_catalogue.terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* def = it.get();
        if (def->typeID == 0)
            continue; // Skip the null terrain def!

        SAVTerrainTypeEntry* entry = terrainTypeTable.append();
        entry->typeID = def->typeID;
        entry->name = writer.append_string(def->name);
    }
    terrainSection.terrainTypeTable = writer.writeArray<SAVTerrainTypeEntry>(terrainTypeTable, terrainTypeTableLoc);

    // Tile terrain type (u8)
    terrainSection.tileTerrainType = writer.appendBlob(&m_tile_terrain_type, FileBlobCompressionScheme::RLE_S8);

    // Tile height (u8)
    terrainSection.tileHeight = writer.appendBlob(&m_tile_height, FileBlobCompressionScheme::RLE_S8);

    // Tile sprite offset (u8)
    terrainSection.tileSpriteOffset = writer.appendBlob(&m_tile_sprite_offset, FileBlobCompressionScheme::Uncompressed);

    writer.endSection<SAVSection_Terrain>(&terrainSection);
}

bool TerrainLayer::load(BinaryFileReader& reader)
{
    bool succeeded = false;

    while (reader.startSection(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION)) // So we can break out of it
    {
        SAVSection_Terrain* section = reader.readStruct<SAVSection_Terrain>(0);
        if (!section)
            break;

        m_terrain_generation_seed = section->terrainGenerationSeed;

        // Map the file's terrain type IDs to the game's ones
        // NB: count+1 because the file won't save the null terrain, so we need to compensate
        Array<u8> oldTypeToNewType = reader.arena->allocate_filled_array<u8>(section->terrainTypeTable.count + 1);
        Array<SAVTerrainTypeEntry> terrainTypeTable = reader.arena->allocate_array<SAVTerrainTypeEntry>(section->terrainTypeTable.count);
        if (!reader.readArray(section->terrainTypeTable, &terrainTypeTable))
            break;
        for (auto const& entry : terrainTypeTable) {
            String terrainName = reader.readString(entry.name);
            oldTypeToNewType[entry.typeID] = findTerrainTypeByName(terrainName);
        }

        // Terrain type
        if (!reader.readBlob(section->tileTerrainType, &m_tile_terrain_type))
            break;
        for (s32 y = 0; y < m_bounds.y(); y++) {
            for (s32 x = 0; x < m_bounds.x(); x++) {
                m_tile_terrain_type.set(x, y, oldTypeToNewType[m_tile_terrain_type.get(x, y)]);
            }
        }

        // Terrain height
        if (!reader.readBlob(section->tileHeight, &m_tile_height))
            break;

        // Sprite offset
        if (!reader.readBlob(section->tileSpriteOffset, &m_tile_sprite_offset))
            break;

        assign_terrain_sprites(m_bounds);
        update_distance_to_water(m_bounds);

        succeeded = true;
        break;
    }

    return succeeded;
}
