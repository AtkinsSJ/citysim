/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/DragState.h>
#include <Sim/Forward.h>
#include <Sim/Terrain.h>

class Tool {
public:
    virtual ~Tool() = default;

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) = 0;
};

class InspectTool final : public Tool {
public:
    // FIXME: This should really go in the InspectWindow once that exists.
    enum class DebugFlags : u8 {
        Fire,
        Power,
        Transport,

        COUNT,
    };
    static Flags<DebugFlags> debug_flags;

    // FIXME: This should REALLY go in InspectWindow, so that we can potentially have multiple of them open inspecting different things.
    static V2I inspected_tile_pos;

    static NonnullOwnPtr<InspectTool> create();
    virtual ~InspectTool() override = default;

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;
};

class BuildTool final : public Tool {
public:
    static NonnullOwnPtr<BuildTool> create(BuildingType);
    virtual ~BuildTool() override = default;

    BuildingType building_type() const { return m_building_type; }

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;

private:
    explicit BuildTool(BuildingType, DragType, V2I building_size);

    BuildingType m_building_type;
    DragState m_drag_state;
};

class DemolishTool final : public Tool {
public:
    static NonnullOwnPtr<DemolishTool> create();
    virtual ~DemolishTool() override = default;

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;

private:
    DragState m_drag_state { DragType::Rect, { 1, 1 } };
};

class ZoneTool final : public Tool {
public:
    static NonnullOwnPtr<ZoneTool> create(ZoneType);
    virtual ~ZoneTool() override = default;

    ZoneType zone_type() const { return m_zone_type; }

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;

private:
    explicit ZoneTool(ZoneType);

    ZoneType m_zone_type;
    DragState m_drag_state { DragType::Rect, { 1, 1 } };
};

class SetTerrainTool final : public Tool {
public:
    static NonnullOwnPtr<SetTerrainTool> create(TerrainType);
    virtual ~SetTerrainTool() override = default;

    TerrainType terrain_type() const { return m_terrain_type; }

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;

private:
    explicit SetTerrainTool(TerrainType);

    TerrainType m_terrain_type;
};

class DebugTool final : public Tool {
public:
    enum class Mode : u8 {
        AddFire,
        RemoveFire,
    };

    static NonnullOwnPtr<DebugTool> create(Mode);
    virtual ~DebugTool() override = default;

    Mode mode() const { return m_mode; }

    virtual void act(City&, bool mouse_is_over_ui, V2I mouse_tile_pos) override;

private:
    explicit DebugTool(Mode);

    Mode m_mode;
};
