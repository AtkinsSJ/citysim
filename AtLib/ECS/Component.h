/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <ECS/Entity.h>
#include <Util/HashMap.h>
#include <Util/OccupancyArray.h>

namespace ECS {

class Component {
};

class BaseComponentTable {
public:
    virtual ~BaseComponentTable() = default;

    virtual Optional<Component&> component_for_entity(Entity) = 0;
};

template<typename T>
class ComponentTable : BaseComponentTable {
public:
    static OwnedRef<ComponentTable> create(MemoryArena& arena)
    {
        return adopt_own(*new ComponentTable { arena });
    }

    virtual ~ComponentTable() override = default;

    virtual Optional<Component&> component_for_entity(Entity entity) override
    {
        if (auto index = m_entity_to_array_index.get(entity); index.has_value())
            return m_components[index.value()];
        return {};
    }

private:
    explicit ComponentTable(MemoryArena& arena)
        : m_components(arena, 512)
    {
    }

    OccupancyArray<T> m_components;
    HashMap<Entity, size_t> m_entity_to_array_index;
};

}
