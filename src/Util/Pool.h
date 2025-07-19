/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/DoublyLinkedList.h>
#include <Util/MemoryArena.h>

template<typename T>
class Poolable;

template<typename T>
class Pool {
public:
    explicit Pool(MemoryArena& arena)
        : m_arena(arena)
    {
    }

    template<typename... Args>
    T& obtain(Args&&... args)
    {
        T* result;
        if (m_available_items.is_empty()) {
            result = &T::allocate_from_pool(m_arena);
            ++m_total_created_item_count;
        } else {
            result = static_cast<T*>(&m_available_items.remove_last());
            --m_available_item_count;
        }

        result->initialize_from_pool(forward<Args>(args)...);
        return *result;
    }

    void discard(T& item)
    {
        item.clear_for_pool();

        m_available_items.add(item);
        ++m_available_item_count;
    }

    u32 available_item_count() const { return m_available_item_count; }
    u32 total_created_item_count() const { return m_total_created_item_count; }

private:
    MemoryArena& m_arena;
    DoublyLinkedList<Poolable<T>> m_available_items;
    u32 m_available_item_count { 0 };
    u32 m_total_created_item_count { 0 };
};

template<typename T>
class Poolable : public DoublyLinkedListNode<Poolable<T>> {
public:
    virtual ~Poolable() = default;

    // NB: Poolable subclasses must also have:
    // - A default constructor.
    // - A `void initialize_from_pool(arguments)` method.

    // NB: Override this to do custom allocation logic.
    static T& allocate_from_pool(MemoryArena& arena)
    {
        return *arena.allocate<T>();
    }

    virtual void clear_for_pool() = 0;
};
