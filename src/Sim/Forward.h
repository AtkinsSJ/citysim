/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

struct Building;
struct BuildingCatalogue;
struct BuildingDef;
class BuildingRef;
struct City;
class CrimeLayer;
class DirtyRects;
class EducationLayer;
class EffectRadius;
struct Entity;
struct Fire;
class FireLayer;
class GameClock;
struct GameState;
class HealthLayer;
class LandValueLayer;
class Layer;
class PollutionLayer;
class PowerLayer;
struct TerrainCatalogue;
struct TerrainDef;
class TerrainLayer;
class TransportLayer;
struct ZoneDef;
class ZoneLayer;

using BuildingType = u32;
enum class ZoneType : u8;
