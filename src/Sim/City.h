/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Crime.h>
#include <Sim/Education.h>
#include <Sim/Entity.h>
#include <Sim/Fire.h>
#include <Sim/Forward.h>
#include <Sim/Health.h>
#include <Sim/LandValue.h>
#include <Sim/Pollution.h>
#include <Sim/Power.h>
#include <Sim/Sector.h>
#include <Sim/Terrain.h>
#include <Sim/Zone.h>
#include <Util/ChunkedArray.h>
#include <Util/OccupancyArray.h>
#include <Util/Rectangle.h>

struct CitySector : public BasicSector {
    // NB: A building is owned by a CitySector if its top-left corner tile is inside that CitySector.
    ChunkedArray<Building*> ownedBuildings;
};

enum class BuildingQueryFlag : u8 {
    RequireOriginInArea, // Only return buildings whose origin (top-left corner) is within the given area.
    COUNT,
};

struct City {
    static NonnullOwnPtr<City> create(MemoryArena&, u32 width, u32 height, String name, String player_name, s32 funds, GameTimestamp date = 0, float time_of_day = 0.0f);

    Building* get_building(BuildingRef const&);
    Building const* get_building(BuildingRef const& ref) const
    {
        return const_cast<City*>(this)->get_building(ref);
    }

    void draw(Rect2I visible_tile_bounds) const;

    void mark_area_dirty(Rect2I);

    bool tile_exists(s32 x, s32 y) const;

    bool building_exists_at(s32 x, s32 y) const;
    Building* get_building_at(s32 x, s32 y);
    Building const* get_building_at(s32 x, s32 y) const
    {
        return const_cast<City*>(this)->get_building_at(x, y);
    }

    void for_each_building_overlapping_area(Rect2I area, Flags<BuildingQueryFlag> flags, Function<void(Building&)> const&);
    void for_each_building_overlapping_area(Rect2I area, Flags<BuildingQueryFlag> flags, Function<void(Building const&)> const&) const;

    void update();

    Building* add_building(BuildingDef* def, Rect2I footprint, Optional<GameTimestamp> const& = {});
    bool can_place_building(BuildingDef* def, s32 left, s32 top) const;
    s32 calculate_build_cost(BuildingDef* def, Rect2I area) const;
    void place_building(BuildingDef* def, s32 left, s32 top, bool markAreasDirty = true);
    void place_building_rect(BuildingDef* def, Rect2I area);

    void save_buildings(BinaryFileWriter* writer) const;
    bool load_buildings(BinaryFileReader* reader);

    s32 calculate_demolition_cost(Rect2I area) const;
    void demolish_rect(Rect2I area);

    template<typename T>
    Entity* add_entity(Entity::Type type, T* entityData)
    {
        Indexed<Entity> entityRecord = entities.append();
        // logInfo("Adding entity #{0}"_s, {formatInt(entityRecord.index)});
        entityRecord.value().index = entityRecord.index();

        Entity& entity = entityRecord.value();
        entity.type = type;
        // Make sure we're supplying entity data that matched the entity type!
        ASSERT(checkEntityMatchesType<T>(&entity));
        entity.dataPointer = entityData;

        entity.color = Colour::white();
        entity.depth = 0;

        entity.canBeDemolished = false;

        return &entity;
    }

    void remove_entity(Entity* entity);
    void draw_entities(Rect2I visibleTileBounds) const;

    // TODO: These are budget related. Separate that out?
    bool can_afford(s32 cost) const;
    void spend(s32 cost);

    void update_adjacent_building_variants(Rect2I footprint);

    // TODO: These want to be in some kind of buffer somewhere so they can be modified!
    String name;
    String playerName;

    GameClock gameClock;
    OwnedPtr<Random> random;

    s32 funds { 0 };
    s32 monthlyExpenditure { 0 };

    Rect2I bounds;

    Array2<s32> tileBuildingIndex; // NB: Index into buildings array, NOT Building.id!
    OccupancyArray<Building> buildings;
    u32 highestBuildingID { 0 };

    SectorGrid<CitySector> sectors;

    OccupancyArray<Entity> entities;

    CrimeLayer crimeLayer;
    EducationLayer educationLayer;
    FireLayer fireLayer;
    HealthLayer healthLayer;
    LandValueLayer landValueLayer;
    PollutionLayer pollutionLayer;
    PowerLayer powerLayer;
    TerrainLayer terrainLayer;
    TransportLayer transportLayer;
    ZoneLayer zoneLayer;

    Array<Layer*> m_layers;

    Rect2I demolitionRect;

    ArrayChunkPool<Building*> sectorBuildingsChunkPool;
    ArrayChunkPool<Rect2I> sectorBoundariesChunkPool;
    ArrayChunkPool<BuildingRef> buildingRefsChunkPool;

private:
    City(MemoryArena&, u32 width, u32 height, String name, String player_name, s32 funds, GameTimestamp date, float time_of_day);

    Building* add_building_direct(s32 id, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate);
};

u8 const maxDistanceToWater = 10;
