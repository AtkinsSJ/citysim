/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Terrain.h"
#include "../AppState.h"
#include "../save_file.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/City.h>
#include <Sim/TerrainCatalogue.h>
#include <UI/Window.h>
#include <Util/Splat.h>

void initTerrainLayer(TerrainLayer* layer, City* city, MemoryArena* gameArena)
{
    layer->tileTerrainType = gameArena->allocate_array_2d<u8>(city->bounds.size());
    layer->tileHeight = gameArena->allocate_array_2d<u8>(city->bounds.size());
    layer->tileDistanceToWater = gameArena->allocate_array_2d<u8>(city->bounds.size());

    layer->tileSpriteOffset = gameArena->allocate_array_2d<u8>(city->bounds.size());
    layer->tileSprite = gameArena->allocate_array_2d<SpriteRef>(city->bounds.size());
    layer->tileBorderSprite = gameArena->allocate_array_2d<SpriteRef>(city->bounds.size());
}

TerrainDef* getTerrainAt(City* city, s32 x, s32 y)
{
    u8 terrainType = city->terrainLayer.tileTerrainType.getIfExists(x, y, 0);

    return getTerrainDef(terrainType);
}

u8 getTerrainHeightAt(City* city, s32 x, s32 y)
{
    return city->terrainLayer.tileHeight.get(x, y);
}

void setTerrainAt(City* city, s32 x, s32 y, u8 terrainType)
{
    u8 existingTerrain = city->terrainLayer.tileTerrainType.getIfExists(x, y, 0);
    // Ignore for tiles that don't exist, or are already the desired type
    if (existingTerrain == 0 || existingTerrain == terrainType) {
        return;
    }

    // Set the terrain
    city->terrainLayer.tileTerrainType.set(x, y, terrainType);

    // Update sprites on this and neighbouring tiles
    Rect2I spriteUpdateBounds = city->bounds.intersected({ x - 1, y - 1, 3, 3 });
    assignTerrainSprites(city, spriteUpdateBounds);

    // Update distance to water
    updateDistanceToWater(city, { x, y, 1, 1 });
}

u8 getDistanceToWaterAt(City* city, s32 x, s32 y)
{
    return city->terrainLayer.tileDistanceToWater.get(x, y);
}

void updateDistanceToWater(City* city, Rect2I dirtyBounds)
{
    TerrainLayer* layer = &city->terrainLayer;
    Rect2I bounds = city->bounds.intersected(dirtyBounds.expanded(maxDistanceToWater));
    u8 tWater = truncate<u8>(findTerrainTypeByName("water"_s));

    for (s32 y = bounds.y();
        y < bounds.y() + bounds.height();
        y++) {
        for (s32 x = bounds.x();
            x < bounds.x() + bounds.width();
            x++) {
            u8 tileType = layer->tileTerrainType.getIfExists(x, y, 0);
            if (tileType == tWater) {
                layer->tileDistanceToWater.set(x, y, 0);
            } else {
                layer->tileDistanceToWater.set(x, y, 255);
            }
        }
    }

    updateDistances(&layer->tileDistanceToWater, bounds, maxDistanceToWater);
}

void drawTerrain(City* city, Rect2I visibleArea, s8 shaderID)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    TerrainLayer* layer = &city->terrainLayer;
    auto& renderer = the_renderer();

    Rect2 spriteBounds { 0.0f, 0.0f, 1.0f, 1.0f };
    auto white = Colour::white();

    // s32 tilesToDraw = areaOf(visibleArea);
    // Asset *terrainTexture = getSprite(getTerrainDef(1)->sprites, 0)->texture;
    // DrawRectsGroup *group = beginRectsGroupTextured(&renderer.world_buffer(), terrainTexture, shaderID, tilesToDraw);

    for (s32 y = visibleArea.y();
        y < visibleArea.y() + visibleArea.height();
        y++) {
        spriteBounds.set_y(y);

        for (s32 x = visibleArea.x();
            x < visibleArea.x() + visibleArea.width();
            x++) {
            Sprite* sprite = getSprite(&layer->tileSprite.get(x, y));
            spriteBounds.set_x(x);
            drawSingleSprite(&renderer.world_buffer(), sprite, spriteBounds, shaderID, white);
            // addSpriteRect(group, sprite, spriteBounds, white);

            SpriteRef* borderSpriteRef = &layer->tileBorderSprite.get(x, y);
            if (!borderSpriteRef->spriteGroupName.is_empty()) {
                Sprite* borderSprite = getSprite(borderSpriteRef);
                drawSingleSprite(&renderer.world_buffer(), borderSprite, spriteBounds, shaderID, white);
                // addSpriteRect(group, borderSprite, spriteBounds, white);
            }
        }
    }
    // endRectsGroup(group);
}

void generateTerrain(City* city, Random* gameRandom)
{
    DEBUG_FUNCTION();

    TerrainLayer* layer = &city->terrainLayer;
    auto& app_state = AppState::the();

    u8 tGround = truncate<u8>(findTerrainTypeByName("ground"_s));
    u8 tWater = truncate<u8>(findTerrainTypeByName("water"_s));
    BuildingDef* treeDef = findBuildingDef("tree"_s);

    s32 seed = gameRandom->next();
    auto* terrainRandom = Random::create(seed);
    layer->terrainGenerationSeed = seed;
    fill<u8>(&layer->tileTerrainType, tGround);

    for (s32 y = 0; y < city->bounds.height(); y++) {
        for (s32 x = 0; x < city->bounds.width(); x++) {
            layer->tileSpriteOffset.set(x, y, app_state.cosmeticRandom->random_integer<u8>());
        }
    }

    // Generate a river
    Array<float> riverOffset = temp_arena().allocate_array<float>(city->bounds.height(), true);
    terrainRandom->fill_with_noise(riverOffset, 10);
    float riverMaxWidth = terrainRandom->random_float_between(12, 16);
    float riverMinWidth = terrainRandom->random_float_between(6, riverMaxWidth);
    float riverWaviness = 16.0f;
    s32 riverCentreBase = terrainRandom->random_between(ceil_s32(city->bounds.width() * 0.4f), floor_s32(city->bounds.width() * 0.6f));
    for (s32 y = 0; y < city->bounds.height(); y++) {
        s32 riverWidth = ceil_s32(lerp(riverMinWidth, riverMaxWidth, ((float)y / (float)city->bounds.height())));
        s32 riverCentre = riverCentreBase - round_s32((riverWaviness * 0.5f) + (riverWaviness * riverOffset[y]));
        s32 riverLeft = riverCentre - (riverWidth / 2);

        for (s32 x = riverLeft; x < riverLeft + riverWidth; x++) {
            layer->tileTerrainType.set(x, y, tWater);
        }
    }

    // Coastline
    Array<float> coastlineOffset = temp_arena().allocate_array<float>(city->bounds.width(), true);
    terrainRandom->fill_with_noise(coastlineOffset, 10);
    for (s32 x = 0; x < city->bounds.width(); x++) {
        s32 coastDepth = 8 + round_s32(coastlineOffset[x] * 16.0f);

        for (s32 i = 0; i < coastDepth; i++) {
            s32 y = city->bounds.height() - 1 - i;

            layer->tileTerrainType.set(x, y, tWater);
        }
    }

    // Lakes/ponds
    s32 pondCount = terrainRandom->random_between(1, 4);
    for (s32 pondIndex = 0; pondIndex < pondCount; pondIndex++) {
        s32 pondCentreX = terrainRandom->random_between(0, city->bounds.width());
        s32 pondCentreY = terrainRandom->random_between(0, city->bounds.height());

        float pondMinRadius = terrainRandom->random_float_between(3.0f, 5.0f);
        float pondMaxRadius = terrainRandom->random_float_between(pondMinRadius + 3.0f, 20.0f);

        Splat pondSplat = Splat::create_random(pondCentreX, pondCentreY, pondMinRadius, pondMaxRadius, 36, terrainRandom);

        Rect2I boundingBox = pondSplat.bounding_box().intersected(city->bounds);
        for (s32 y = boundingBox.y(); y < boundingBox.y() + boundingBox.height(); y++) {
            for (s32 x = boundingBox.x(); x < boundingBox.x() + boundingBox.width(); x++) {
                if (pondSplat.contains(x, y)) {
                    layer->tileTerrainType.set(x, y, tWater);
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
            s32 centreX = terrainRandom->random_between(0, city->bounds.width());
            s32 centreY = terrainRandom->random_between(0, city->bounds.height());

            float minRadius = terrainRandom->random_float_between(2.0f, 8.0f);
            float maxRadius = terrainRandom->random_float_between(minRadius + 1.0f, 30.0f);

            Splat forestSplat = Splat::create_random(centreX, centreY, minRadius, maxRadius, 36, terrainRandom);

            Rect2I boundingBox = forestSplat.bounding_box().intersected(city->bounds);
            for (s32 y = boundingBox.y(); y < boundingBox.y() + boundingBox.height(); y++) {
                for (s32 x = boundingBox.x(); x < boundingBox.x() + boundingBox.width(); x++) {
                    if (getTerrainAt(city, x, y)->canBuildOn
                        && (getBuildingAt(city, x, y) == nullptr)
                        && forestSplat.contains(x, y)) {
                        addBuilding(city, treeDef, { x, y, treeDef->size.x, treeDef->size.y });
                    }
                }
            }
        }
    }

    assignTerrainSprites(city, city->bounds);

    updateDistanceToWater(city, city->bounds);
}

void assignTerrainSprites(City* city, Rect2I bounds)
{
    TerrainLayer* layer = &city->terrainLayer;

    // Assign a terrain tile variant for each tile, depending on its neighbours
    for (s32 y = bounds.y();
        y < bounds.y() + bounds.height();
        y++) {
        for (s32 x = bounds.x();
            x < bounds.x() + bounds.width();
            x++) {
            TerrainDef* def = getTerrainAt(city, x, y);

            layer->tileSprite.set(x, y, getSpriteRef(def->spriteName, layer->tileSpriteOffset.get(x, y)));

            if (def->drawBordersOver) {
                // First, determine if we have a bordering type
                TerrainDef* borders[8] = {}; // Starting NW, going clockwise
                borders[0] = getTerrainAt(city, x - 1, y - 1);
                borders[1] = getTerrainAt(city, x, y - 1);
                borders[2] = getTerrainAt(city, x + 1, y - 1);
                borders[3] = getTerrainAt(city, x + 1, y);
                borders[4] = getTerrainAt(city, x + 1, y + 1);
                borders[5] = getTerrainAt(city, x, y + 1);
                borders[6] = getTerrainAt(city, x - 1, y + 1);
                borders[7] = getTerrainAt(city, x - 1, y);

                // Find the first match, if any
                // Eventually we probably want to use whichever border terrain is most common instead
                TerrainDef* borderTerrain = nullptr;
                for (s32 i = 0; i < 8; i++) {
                    if ((borders[i] != def) && (!borders[i]->borderSpriteNames.isEmpty())) {
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
                    layer->tileBorderSprite.set(x, y, getSpriteRef(borderTerrain->borderSpriteNames[borderSpriteIndex], layer->tileSpriteOffset.get(x, y)));
                } else {
                    layer->tileBorderSprite.set(x, y, {});
                }
            } else {
                layer->tileBorderSprite.set(x, y, {});
            }
        }
    }
}

void showTerrainWindow()
{
    UI::showWindow(UI::WindowTitle::fromTextAsset("title_terrain"_s), 300, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::UniqueKeepPosition | WindowFlags::AutomaticHeight, modifyTerrainWindowProc);
}

void modifyTerrainWindowProc(UI::WindowContext* context, void*)
{
    UI::Panel* ui = &context->windowPanel;
    GameState* gameState = AppState::the().gameState;
    bool terrainToolIsActive = (gameState->actionMode == ActionMode::SetTerrain);

    for (auto it = TerrainCatalogue::the().terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* terrain = it.get();
        if (terrain->typeID == 0)
            continue; // Skip the null terrain

        if (ui->addImageButton(getSprite(terrain->spriteName, 1), buttonIsActive(terrainToolIsActive && gameState->selectedTerrainID == terrain->typeID))) {
            gameState->actionMode = ActionMode::SetTerrain;
            gameState->selectedTerrainID = terrain->typeID;
        }
    }
}

void saveTerrainLayer(TerrainLayer* layer, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Terrain>(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION);
    SAVSection_Terrain terrainSection = {};

    // Terrain generation parameters
    terrainSection.terrainGenerationSeed = layer->terrainGenerationSeed;

    // Terrain types table
    auto& terrain_catalogue = TerrainCatalogue::the();
    s32 terrainDefCount = terrain_catalogue.terrainDefs.count - 1; // Skip the null def
    WriteBufferRange terrainTypeTableLoc = writer->reserveArray<SAVTerrainTypeEntry>(terrainDefCount);
    Array<SAVTerrainTypeEntry> terrainTypeTable = writer->arena->allocate_array<SAVTerrainTypeEntry>(terrainDefCount);
    for (auto it = terrain_catalogue.terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* def = it.get();
        if (def->typeID == 0)
            continue; // Skip the null terrain def!

        SAVTerrainTypeEntry* entry = terrainTypeTable.append();
        entry->typeID = def->typeID;
        entry->name = writer->appendString(def->name);
    }
    terrainSection.terrainTypeTable = writer->writeArray<SAVTerrainTypeEntry>(terrainTypeTable, terrainTypeTableLoc);

    // Tile terrain type (u8)
    terrainSection.tileTerrainType = writer->appendBlob(&layer->tileTerrainType, FileBlobCompressionScheme::RLE_S8);

    // Tile height (u8)
    terrainSection.tileHeight = writer->appendBlob(&layer->tileHeight, FileBlobCompressionScheme::RLE_S8);

    // Tile sprite offset (u8)
    terrainSection.tileSpriteOffset = writer->appendBlob(&layer->tileSpriteOffset, FileBlobCompressionScheme::Uncompressed);

    writer->endSection<SAVSection_Terrain>(&terrainSection);
}

bool loadTerrainLayer(TerrainLayer* layer, City* city, BinaryFileReader* reader)
{
    bool succeeded = false;

    while (reader->startSection(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION)) // So we can break out of it
    {
        SAVSection_Terrain* section = reader->readStruct<SAVSection_Terrain>(0);
        if (!section)
            break;

        layer->terrainGenerationSeed = section->terrainGenerationSeed;

        // Map the file's terrain type IDs to the game's ones
        // NB: count+1 because the file won't save the null terrain, so we need to compensate
        Array<u8> oldTypeToNewType = reader->arena->allocate_array<u8>(section->terrainTypeTable.count + 1, true);
        Array<SAVTerrainTypeEntry> terrainTypeTable = reader->arena->allocate_array<SAVTerrainTypeEntry>(section->terrainTypeTable.count);
        if (!reader->readArray(section->terrainTypeTable, &terrainTypeTable))
            break;
        for (s32 i = 0; i < terrainTypeTable.count; i++) {
            SAVTerrainTypeEntry* entry = &terrainTypeTable[i];
            String terrainName = reader->readString(entry->name);
            oldTypeToNewType[entry->typeID] = findTerrainTypeByName(terrainName);
        }

        // Terrain type
        if (!reader->readBlob(section->tileTerrainType, &layer->tileTerrainType))
            break;
        for (s32 y = 0; y < city->bounds.y(); y++) {
            for (s32 x = 0; x < city->bounds.x(); x++) {
                layer->tileTerrainType.set(x, y, oldTypeToNewType[layer->tileTerrainType.get(x, y)]);
            }
        }

        // Terrain height
        if (!reader->readBlob(section->tileHeight, &layer->tileHeight))
            break;

        // Sprite offset
        if (!reader->readBlob(section->tileSpriteOffset, &layer->tileSpriteOffset))
            break;

        assignTerrainSprites(city, city->bounds);
        updateDistanceToWater(city, city->bounds);

        succeeded = true;
        break;
    }

    return succeeded;
}
