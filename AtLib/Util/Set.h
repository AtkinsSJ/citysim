/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/ChunkedArray.h>

template<typename T>
struct SetIterator;

template<typename T>
class Set {
public:
    // FIXME: Temporary until initialization is more sane everywhere.
    Set() = default;

    using ComparisonFunction = Function<bool(T*, T*)>;

    explicit Set(MemoryArena& arena, ComparisonFunction are_items_equal = [](T* a, T* b) { return *a == *b; })
        : m_items(arena, 64) // 64 is an arbitrary choice!
        , m_are_items_equal(are_items_equal)
    {
    }

    // Returns true if the item got added (aka, it was not already in the set)
    bool add(T item)
    {
        bool didAdd = false;

        if (!contains(item)) {
            m_items.append(item);
            didAdd = true;
        }

        return didAdd;
    }

    bool remove(T item)
    {
        bool didRemove = false;

        s32 removed = m_items.removeAll([&](T* t) { return m_are_items_equal(&item, t); }, 1);
        didRemove = (removed == 1);

        return didRemove;
    }

    bool contains(T item)
    {
        bool result = false;

        for (auto it = m_items.iterate(); it.hasNext(); it.next()) {
            if (m_are_items_equal(&item, &it.get())) {
                result = true;
                break;
            }
        }

        return result;
    }

    bool is_empty() const { return m_items.is_empty(); }
    void clear() { m_items.clear(); }

    SetIterator<T> iterate()
    {
        SetIterator<T> result = {};
        result.itemsIterator = m_items.iterate();

        return result;
    }

    // The compare() function should return true if 'a' comes before 'b'
    Array<T> asSortedArray(bool (*compare)(T a, T b) = [](T a, T b) { return a < b; })
    {
        if (is_empty())
            return {};

        auto result = temp_arena().allocate_array<T>(m_items.count);

        // Gather
        for (auto it = iterate(); it.hasNext(); it.next()) {
            result.append(it.getValue());
        }

        // Sort
        result.sort(compare);
        return result;
    }

private:
    // TODO: @Speed: This is a really dumb, linearly-compare-with-all-the-elements implementation!
    // Replace it with a binary tree or some kind of hash map or something, if we end up using this a lot.
    // For now, simple is better!
    // - Sam, 03/09/2019
    ChunkedArray<T> m_items;
    ComparisonFunction m_are_items_equal;
};

template<typename T>
struct SetIterator {
    // This is SUPER JANKY
    ChunkedArrayIterator<T> itemsIterator;

    // Methods
    bool hasNext() { return !itemsIterator.isDone; }
    void next() { itemsIterator.next(); }
    T* get() { return itemsIterator.get(); }
    T getValue() { return itemsIterator.getValue(); }
};
