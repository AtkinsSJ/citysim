/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "terrain.h"
#include "AppState.h"
#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "city.h"
#include "line_reader.h"
#include "render.h"
#include "save_file.h"
#include "splat.h"
#include <Assets/AssetManager.h>
#include <UI/Window.h>

TerrainCatalogue s_terrain_catalogue = {};

void initTerrainLayer(TerrainLayer* layer, City* city, MemoryArena* gameArena)
{
    layer->tileTerrainType = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
    layer->tileHeight = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
    layer->tileDistanceToWater = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);

    layer->tileSpriteOffset = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
    layer->tileSprite = allocateArray2<SpriteRef>(gameArena, city->bounds.w, city->bounds.h);
    layer->tileBorderSprite = allocateArray2<SpriteRef>(gameArena, city->bounds.w, city->bounds.h);
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
    Rect2I spriteUpdateBounds = intersect(irectXYWH(x - 1, y - 1, 3, 3), city->bounds);
    assignTerrainSprites(city, spriteUpdateBounds);

    // Update distance to water
    updateDistanceToWater(city, irectXYWH(x, y, 1, 1));
}

 u8 getDistanceToWaterAt(City* city, s32 x, s32 y)
{
    return city->terrainLayer.tileDistanceToWater.get(x, y);
}

void updateDistanceToWater(City* city, Rect2I dirtyBounds)
{
    TerrainLayer* layer = &city->terrainLayer;
    Rect2I bounds = intersect(expand(dirtyBounds, maxDistanceToWater), city->bounds);
    u8 tWater = truncate<u8>(findTerrainTypeByName("water"_s));

    for (s32 y = bounds.y;
        y < bounds.y + bounds.h;
        y++) {
        for (s32 x = bounds.x;
            x < bounds.x + bounds.w;
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

void initTerrainCatalogue()
{
    s_terrain_catalogue = {};

    initOccupancyArray(&s_terrain_catalogue.terrainDefs, &AppState::the().systemArena, 128);
    Indexed<TerrainDef*> nullTerrainDef = s_terrain_catalogue.terrainDefs.append();
    *nullTerrainDef.value = {};

    initHashTable(&s_terrain_catalogue.terrainDefsByName, 0.75f, 128);
    initStringTable(&s_terrain_catalogue.terrainNames);

    initHashTable(&s_terrain_catalogue.terrainNameToType, 0.75f, 128);
    s_terrain_catalogue.terrainNameToType.put(nullString, 0);

    initHashTable(&s_terrain_catalogue.terrainNameToOldType, 0.75f, 128);
}

void loadTerrainDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader = readLines(asset->shortName, data);

    // Pre scan for the number of Terrains, so we can allocate enough space in the asset.
    s32 terrainCount = 0;
    while (loadNextLine(&reader)) {
        String command = readToken(&reader);
        if (equals(command, ":Terrain"_s)) {
            terrainCount++;
        }
    }

    asset->data = assetsAllocate(&asset_manager(), sizeof(String) * terrainCount);
    asset->terrainDefs.terrainIDs = makeArray(terrainCount, (String*)asset->data.memory);

    restart(&reader);

    TerrainDef* def = nullptr;

    while (loadNextLine(&reader)) {
        String firstWord = readToken(&reader);

        if (firstWord.chars[0] == ':') // Definitions
        {
            // Define something
            firstWord.chars++;
            firstWord.length--;

            if (equals(firstWord, "Terrain"_s)) {
                String name = readToken(&reader);
                if (isEmpty(name)) {
                    error(&reader, "Couldn't parse Terrain. Expected: ':Terrain identifier'"_s);
                    return;
                }

                Indexed<TerrainDef*> slot = s_terrain_catalogue.terrainDefs.append();
                def = slot.value;
                *def = {};

                if (slot.index > u8Max) {
                    error(&reader, "Too many Terrain definitions! The most we support is {0}."_s, { formatInt(u8Max) });
                    return;
                }
                def->typeID = (u8)slot.index;

                def->name = intern(&s_terrain_catalogue.terrainNames, name);
                asset->terrainDefs.terrainIDs.append(def->name);
                s_terrain_catalogue.terrainDefsByName.put(def->name, def);
                s_terrain_catalogue.terrainNameToType.put(def->name, def->typeID);
            } else {
                error(&reader, "Unrecognised command: '{0}'"_s, { firstWord });
            }
        } else // Properties!
        {
            if (def == nullptr) {
                error(&reader, "Found a property before starting a :Terrain!"_s);
                return;
            } else if (equals(firstWord, "borders"_s)) {
                def->borderSpriteNames = allocateArray<String>(&asset_manager().assetArena, 80);
            } else if (equals(firstWord, "border"_s)) {
                def->borderSpriteNames.append(intern(&asset_manager().assetStrings, readToken(&reader)));
            } else if (equals(firstWord, "can_build_on"_s)) {
                Maybe<bool> boolRead = readBool(&reader);
                if (boolRead.isValid)
                    def->canBuildOn = boolRead.value;
            } else if (equals(firstWord, "draw_borders_over"_s)) {
                Maybe<bool> boolRead = readBool(&reader);
                if (boolRead.isValid)
                    def->drawBordersOver = boolRead.value;
            } else if (equals(firstWord, "name"_s)) {
                def->textAssetName = intern(&asset_manager().assetStrings, readToken(&reader));
            } else if (equals(firstWord, "sprite"_s)) {
                def->spriteName = intern(&asset_manager().assetStrings, readToken(&reader));
            } else {
                warn(&reader, "Unrecognised property '{0}' inside command ':Terrain'"_s, { firstWord });
            }
        }
    }
}

void removeTerrainDefs(Array<String> namesToRemove)
{
    for (s32 nameIndex = 0; nameIndex < namesToRemove.count; nameIndex++) {
        String terrainName = namesToRemove[nameIndex];
        s32 terrainIndex = findTerrainTypeByName(terrainName);
        if (terrainIndex > 0) {
            s_terrain_catalogue.terrainDefs.removeIndex(terrainIndex);

            s_terrain_catalogue.terrainNameToType.removeKey(terrainName);
        }
    }
}

 TerrainDef* getTerrainDef(u8 terrainType)
{
    TerrainDef* result = s_terrain_catalogue.terrainDefs.get(0);

    if (terrainType > 0 && terrainType < s_terrain_catalogue.terrainDefs.count) {
        TerrainDef* found = s_terrain_catalogue.terrainDefs.get(terrainType);
        if (found != nullptr)
            result = found;
    }

    return result;
}

u8 findTerrainTypeByName(String name)
{
    DEBUG_FUNCTION();

    u8 result = 0;

    Maybe<TerrainDef*> def = s_terrain_catalogue.terrainDefsByName.findValue(name);
    if (def.isValid && def.value != nullptr) {
        result = def.value->typeID;
    }

    return result;
}

void drawTerrain(City* city, Rect2I visibleArea, s8 shaderID)
{
    DEBUG_FUNCTION_T(DCDT_GameUpdate);

    TerrainLayer* layer = &city->terrainLayer;
    auto* renderer = the_renderer();

    Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
    V4 white = makeWhite();

    // s32 tilesToDraw = areaOf(visibleArea);
    // Asset *terrainTexture = getSprite(getTerrainDef(1)->sprites, 0)->texture;
    // DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, terrainTexture, shaderID, tilesToDraw);

    for (s32 y = visibleArea.y;
        y < visibleArea.y + visibleArea.h;
        y++) {
        spriteBounds.y = (f32)y;

        for (s32 x = visibleArea.x;
            x < visibleArea.x + visibleArea.w;
            x++) {
            Sprite* sprite = getSprite(&layer->tileSprite.get(x, y));
            spriteBounds.x = (f32)x;
            drawSingleSprite(&renderer->worldBuffer, sprite, spriteBounds, shaderID, white);
            // addSpriteRect(group, sprite, spriteBounds, white);

            SpriteRef* borderSpriteRef = &layer->tileBorderSprite.get(x, y);
            if (!borderSpriteRef->spriteGroupName.isEmpty()) {
                Sprite* borderSprite = getSprite(borderSpriteRef);
                drawSingleSprite(&renderer->worldBuffer, borderSprite, spriteBounds, shaderID, white);
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

    Random terrainRandom;
    s32 seed = randomNext(gameRandom);
    layer->terrainGenerationSeed = seed;
    initRandom(&terrainRandom, Random_MT, seed);
    fill<u8>(&layer->tileTerrainType, tGround);

    for (s32 y = 0; y < city->bounds.h; y++) {
        for (s32 x = 0; x < city->bounds.w; x++) {
            layer->tileSpriteOffset.set(x, y, randomInRange<u8>(&app_state.cosmeticRandom));
        }
    }

    // Generate a river
    Array<f32> riverOffset = allocateArray<f32>(&temp_arena(), city->bounds.h, true);
    generate1DNoise(&terrainRandom, &riverOffset, 10);
    f32 riverMaxWidth = randomFloatBetween(&terrainRandom, 12, 16);
    f32 riverMinWidth = randomFloatBetween(&terrainRandom, 6, riverMaxWidth);
    f32 riverWaviness = 16.0f;
    s32 riverCentreBase = randomBetween(&terrainRandom, ceil_s32(city->bounds.w * 0.4f), floor_s32(city->bounds.w * 0.6f));
    for (s32 y = 0; y < city->bounds.h; y++) {
        s32 riverWidth = ceil_s32(lerp(riverMinWidth, riverMaxWidth, ((f32)y / (f32)city->bounds.h)));
        s32 riverCentre = riverCentreBase - round_s32((riverWaviness * 0.5f) + (riverWaviness * riverOffset[y]));
        s32 riverLeft = riverCentre - (riverWidth / 2);

        for (s32 x = riverLeft; x < riverLeft + riverWidth; x++) {
            layer->tileTerrainType.set(x, y, tWater);
        }
    }

    // Coastline
    Array<f32> coastlineOffset = allocateArray<f32>(&temp_arena(), city->bounds.w, true);
    generate1DNoise(&terrainRandom, &coastlineOffset, 10);
    for (s32 x = 0; x < city->bounds.w; x++) {
        s32 coastDepth = 8 + round_s32(coastlineOffset[x] * 16.0f);

        for (s32 i = 0; i < coastDepth; i++) {
            s32 y = city->bounds.h - 1 - i;

            layer->tileTerrainType.set(x, y, tWater);
        }
    }

    // Lakes/ponds
    s32 pondCount = randomBetween(&terrainRandom, 1, 4);
    for (s32 pondIndex = 0; pondIndex < pondCount; pondIndex++) {
        s32 pondCentreX = randomBetween(&terrainRandom, 0, city->bounds.w);
        s32 pondCentreY = randomBetween(&terrainRandom, 0, city->bounds.h);

        f32 pondMinRadius = randomFloatBetween(&terrainRandom, 3.0f, 5.0f);
        f32 pondMaxRadius = randomFloatBetween(&terrainRandom, pondMinRadius + 3.0f, 20.0f);

        Splat pondSplat = createRandomSplat(pondCentreX, pondCentreY, pondMinRadius, pondMaxRadius, 36, &terrainRandom);

        Rect2I boundingBox = intersect(getBoundingBox(&pondSplat), city->bounds);
        for (s32 y = boundingBox.y; y < boundingBox.y + boundingBox.h; y++) {
            for (s32 x = boundingBox.x; x < boundingBox.x + boundingBox.w; x++) {
                if (contains(&pondSplat, x, y)) {
                    layer->tileTerrainType.set(x, y, tWater);
                }
            }
        }
    }

    // Forest splats
    if (treeDef == nullptr) {
        logError("Map generator is unable to place any trees, because the 'tree' building was not found."_s);
    } else {
        s32 forestCount = randomBetween(&terrainRandom, 10, 20);
        for (s32 forestIndex = 0; forestIndex < forestCount; forestIndex++) {
            s32 centreX = randomBetween(&terrainRandom, 0, city->bounds.w);
            s32 centreY = randomBetween(&terrainRandom, 0, city->bounds.h);

            f32 minRadius = randomFloatBetween(&terrainRandom, 2.0f, 8.0f);
            f32 maxRadius = randomFloatBetween(&terrainRandom, minRadius + 1.0f, 30.0f);

            Splat forestSplat = createRandomSplat(centreX, centreY, minRadius, maxRadius, 36, &terrainRandom);

            Rect2I boundingBox = intersect(getBoundingBox(&forestSplat), city->bounds);
            for (s32 y = boundingBox.y; y < boundingBox.y + boundingBox.h; y++) {
                for (s32 x = boundingBox.x; x < boundingBox.x + boundingBox.w; x++) {
                    if (getTerrainAt(city, x, y)->canBuildOn
                        && (getBuildingAt(city, x, y) == nullptr)
                        && contains(&forestSplat, x, y)) {
                        addBuilding(city, treeDef, irectXYWH(x, y, treeDef->size.x, treeDef->size.y));
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
    for (s32 y = bounds.y;
        y < bounds.y + bounds.h;
        y++) {
        for (s32 x = bounds.x;
            x < bounds.x + bounds.w;
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

void saveTerrainTypes()
{
    s_terrain_catalogue.terrainNameToOldType.putAll(&s_terrain_catalogue.terrainNameToType);
}

void remapTerrainTypes(City* city)
{
    // First, remap any Names that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        if (!s_terrain_catalogue.terrainNameToType.contains(entry->key)) {
            s_terrain_catalogue.terrainNameToType.put(entry->key, (u8)s_terrain_catalogue.terrainNameToType.count);
        }
    }

    if (s_terrain_catalogue.terrainNameToOldType.count > 0) {
        Array<u8> oldTypeToNewType = allocateArray<u8>(&temp_arena(), s_terrain_catalogue.terrainNameToOldType.count, true);
        for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String terrainName = entry->key;
            u8 oldType = entry->value;

            oldTypeToNewType[oldType] = s_terrain_catalogue.terrainNameToType.findValue(terrainName).orDefault(0);
        }

        TerrainLayer* layer = &city->terrainLayer;

        for (s32 y = 0; y < layer->tileTerrainType.h; y++) {
            for (s32 x = 0; x < layer->tileTerrainType.w; x++) {
                u8 oldType = layer->tileTerrainType.get(x, y);

                if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0)) {
                    layer->tileTerrainType.set(x, y, oldTypeToNewType[oldType]);
                }
            }
        }
    }

    saveTerrainTypes();
}

void showTerrainWindow()
{
    UI::showWindow(UI::WindowTitle::fromTextAsset("title_terrain"_s), 300, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::AutomaticHeight, modifyTerrainWindowProc);
}

void modifyTerrainWindowProc(UI::WindowContext* context, void*)
{
    UI::Panel* ui = &context->windowPanel;
    GameState* gameState = AppState::the().gameState;
    bool terrainToolIsActive = (gameState->actionMode == ActionMode_SetTerrain);

    for (auto it = s_terrain_catalogue.terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* terrain = it.get();
        if (terrain->typeID == 0)
            continue; // Skip the null terrain

        if (ui->addImageButton(getSprite(terrain->spriteName, 1), buttonIsActive(terrainToolIsActive && gameState->selectedTerrainID == terrain->typeID))) {
            gameState->actionMode = ActionMode_SetTerrain;
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
    s32 terrainDefCount = s_terrain_catalogue.terrainDefs.count - 1; // Skip the null def
    WriteBufferRange terrainTypeTableLoc = writer->reserveArray<SAVTerrainTypeEntry>(terrainDefCount);
    Array<SAVTerrainTypeEntry> terrainTypeTable = allocateArray<SAVTerrainTypeEntry>(writer->arena, terrainDefCount);
    for (auto it = s_terrain_catalogue.terrainDefs.iterate(); it.hasNext(); it.next()) {
        TerrainDef* def = it.get();
        if (def->typeID == 0)
            continue; // Skip the null terrain def!

        SAVTerrainTypeEntry* entry = terrainTypeTable.append();
        entry->typeID = def->typeID;
        entry->name = writer->appendString(def->name);
    }
    terrainSection.terrainTypeTable = writer->writeArray<SAVTerrainTypeEntry>(terrainTypeTable, terrainTypeTableLoc);

    // Tile terrain type (u8)
    terrainSection.tileTerrainType = writer->appendBlob(&layer->tileTerrainType, Blob_RLE_S8);

    // Tile height (u8)
    terrainSection.tileHeight = writer->appendBlob(&layer->tileHeight, Blob_RLE_S8);

    // Tile sprite offset (u8)
    terrainSection.tileSpriteOffset = writer->appendBlob(&layer->tileSpriteOffset, Blob_Uncompressed);

    writer->endSection<SAVSection_Terrain>(&terrainSection);
}

bool loadTerrainLayer(TerrainLayer* layer, City* city, struct BinaryFileReader* reader)
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
        Array<u8> oldTypeToNewType = allocateArray<u8>(reader->arena, section->terrainTypeTable.count + 1, true);
        Array<SAVTerrainTypeEntry> terrainTypeTable = allocateArray<SAVTerrainTypeEntry>(reader->arena, section->terrainTypeTable.count);
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
        for (s32 y = 0; y < city->bounds.y; y++) {
            for (s32 x = 0; x < city->bounds.x; x++) {
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
