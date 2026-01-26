/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/GameClock.h>
#include <Sim/Sector.h>
#include <UI/Forward.h>
#include <Util/ChunkedArray.h>
#include <Util/Forward.h>
#include <Util/Rectangle.h>
#include <Util/Vector.h>

struct Entity;

struct Fire {
    Entity* entity;

    V2I pos;
    // TODO: severity
    // TODO: Timing information (fires shouldn't last forever!)
    GameTimestamp startDate;
};

struct FireSector {
    Rect2I bounds;

    ChunkedArray<Fire> activeFires;
};

struct FireLayer {
    DirtyRects dirtyRects;

    SectorGrid<FireSector> sectors;

    u8 maxFireRadius;
    Array2<u16> tileFireProximityEffect;

    Array2<u8> tileTotalFireRisk; // Risks combined

    Array2<u8> tileFireProtection;

    Array2<u8> tileOverallFireRisk; // Risks after we've taken protection into account

    ChunkedArray<BuildingRef> fireProtectionBuildings;

    ArrayChunkPool<Fire> firePool;
    s32 activeFireCount;

    float fundingLevel; // @Budget

    // Debug stuff
    V2I debugTileInspectionPos;
};

void initFireLayer(FireLayer* layer, City* city, MemoryArena* gameArena);
void updateFireLayer(City* city, FireLayer* layer);
void markFireLayerDirty(FireLayer* layer, Rect2I bounds);

void notifyNewBuilding(FireLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(FireLayer* layer, BuildingDef* def, Building* building);

Optional<Indexed<Fire>> find_fire_at(City* city, s32 x, s32 y);
bool doesAreaContainFire(City* city, Rect2I bounds);
void startFireAt(City* city, s32 x, s32 y);
void addFireRaw(City* city, s32 x, s32 y, GameTimestamp startDate);
void updateFire(City* city, Fire* fire);
void removeFireAt(City* city, s32 x, s32 y);

u8 getFireRiskAt(City* city, s32 x, s32 y);
u8 getFireProtectionAt(City* city, s32 x, s32 y);
float getFireProtectionPercentAt(City* city, s32 x, s32 y);

void debugInspectFire(UI::Panel* panel, City* city, s32 x, s32 y);

void saveFireLayer(FireLayer* layer, BinaryFileWriter* writer);
bool loadFireLayer(FireLayer* layer, City* city, BinaryFileReader* reader);
