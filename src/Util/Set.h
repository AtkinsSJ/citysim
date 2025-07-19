/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "ChunkedArray.h"

template<typename T>
struct SetIterator;

template<typename T>
struct Set {
    // TODO: @Speed: This is a really dumb, linearly-compare-with-all-the-elements implementation!
    // Replace it with a binary tree or some kind of hash map or something, if we end up using this a lot.
    // For now, simple is better!
    // - Sam, 03/09/2019
    ChunkedArray<T> items;
    bool (*areItemsEqual)(T* a, T* b);

    // Methods

    // Returns true if the item got added (aka, it was not already in the set)
    bool add(T item)
    {
        bool didAdd = false;

        if (!contains(item)) {
            items.append(item);
            didAdd = true;
        }

        return didAdd;
    }

    bool remove(T item)
    {
        bool didRemove = false;

        s32 removed = items.removeAll([&](T* t) { return areItemsEqual(&item, t); }, 1);
        didRemove = (removed == 1);

        return didRemove;
    }

    bool contains(T item)
    {
        bool result = false;

        for (auto it = items.iterate(); it.hasNext(); it.next()) {
            if (areItemsEqual(&item, it.get())) {
                result = true;
                break;
            }
        }

        return result;
    }

    bool isEmpty() { return items.isEmpty(); }
    void clear() { items.clear(); }

    SetIterator<T> iterate()
    {
        SetIterator<T> result = {};
        result.itemsIterator = items.iterate();

        return result;
    }

    // The compare() function should return true if 'a' comes before 'b'
    Array<T> asSortedArray(bool (*compare)(T a, T b) = [](T a, T b) { return a < b; })
    {
        Array<T> result = makeEmptyArray<T>();

        if (!isEmpty()) {
            result = temp_arena().allocate_array<T>(items.count, false);

            // Gather
            for (auto it = iterate(); it.hasNext(); it.next()) {
                result.append(it.getValue());
            }

            // Sort
            result.sort(compare);
        }

        return result;
    }
};

template<typename T>
void initSet(Set<T>* set, MemoryArena* arena, bool (*areItemsEqual)(T* a, T* b) = [](T* a, T* b) { return *a == *b; })
{
    *set = {};
    initChunkedArray(&set->items, arena, 64); // 64 is an arbitrary choice!
    set->areItemsEqual = areItemsEqual;
}

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
