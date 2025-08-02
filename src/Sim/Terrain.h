/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Sprite.h>
#include <IO/Forward.h>
#include <Sim/Forward.h>
#include <UI/Forward.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct TerrainLayer {
    s32 terrainGenerationSeed;

    Array2<u8> tileTerrainType;
    Array2<u8> tileHeight;
    Array2<u8> tileDistanceToWater;

    Array2<u8> tileSpriteOffset;
    Array2<SpriteRef> tileSprite;
    Array2<SpriteRef> tileBorderSprite;
};

void initTerrainLayer(TerrainLayer* layer, City* city, MemoryArena* gameArena);

void generateTerrain(City* city, Random* gameRandom);
void drawTerrain(City* city, Rect2I visibleArea, s8 shaderID);
TerrainDef* getTerrainAt(City* city, s32 x, s32 y);
void setTerrainAt(City* city, s32 x, s32 y, u8 terrainType);

u8 getDistanceToWaterAt(City* city, s32 x, s32 y);
void updateDistanceToWater(City* city, Rect2I bounds);

void assignTerrainSprites(City* city, Rect2I bounds);

void showTerrainWindow();
void modifyTerrainWindowProc(UI::WindowContext*, void*);

void saveTerrainLayer(TerrainLayer* layer, BinaryFileWriter* writer);
bool loadTerrainLayer(TerrainLayer* layer, City* city, BinaryFileReader* reader);
