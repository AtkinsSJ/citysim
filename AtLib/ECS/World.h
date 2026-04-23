/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <ECS/Component.h>
#include <Util/HashMap.h>
#include <Util/OwnedPtr.h>
#include <typeindex>

namespace ECS {

class World {
public:
    template<typename T>
    void register_component()
    {
        auto type_index = std::type_index(typeid(T));
        m_components.ensure(type_index, [this] {
            return adopt_own(new ComponentTable<T> { m_arena });
        });
    }

    template<typename T>
    ComponentTable<T>& component_table()
    {
        if (auto component_table = m_components.get(std::type_index(typeid(T))); component_table.has_value()) {
            return dynamic_cast<ComponentTable<T>>(*component_table.value());
        }

        return {};
    }

private:
    MemoryArena m_arena { "ECS::World"_s };
    HashMap<std::type_index, OwnedRef<BaseComponentTable>> m_components;
};

}
