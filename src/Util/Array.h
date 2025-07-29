/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>

template<typename T>
struct Array {
    s32 capacity;
    s32 count;
    T* items;

    // NB: it's a reference so you can do assignments!
    T& operator[](s32 index)
    {
        ASSERT(index >= 0 && index < count); // Index out of range!
        return items[index];
    }

    // Same as [] but easier when we're using an Array*
    T* get(s32 index)
    {
        ASSERT(index >= 0 && index < count); // Index out of range!
        return &items[index];
    }

    T* first()
    {
        ASSERT(count > 0); // Index out of range!
        return &items[0];
    }

    T* last()
    {
        ASSERT(count > 0); // Index out of range!
        return &items[count - 1];
    }

    T* append()
    {
        ASSERT(count < capacity);

        T* result = items + count++;

        return result;
    }

    T* append(T item)
    {
        T* result = append();
        *result = item;
        return result;
    }

    bool isInitialised() { return items != nullptr; }
    bool isEmpty() { return count == 0; }
    bool hasRoom() { return count < capacity; }

    void swap(s32 indexA, s32 indexB)
    {
        T temp = items[indexA];
        items[indexA] = items[indexB];
        items[indexB] = temp;
    }

    // compareElements(T a, T b) -> returns (a < b), to sort low to high
    template<typename Comparison>
    void sort(Comparison compareElements)
    {
        sortInternal(compareElements, 0, count - 1);
    }

    template<typename Comparison>
    void sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex)
    {
        // Quicksort implementation
        if (lowIndex < highIndex) {
            s32 partitionIndex = 0;
            {
                T pivot = items[highIndex];
                s32 i = (lowIndex - 1);
                for (s32 j = lowIndex; j < highIndex; j++) {
                    if (compareElements(items[j], pivot)) {
                        i++;
                        swap(i, j);
                    }
                }
                swap(i + 1, highIndex);
                partitionIndex = i + 1;
            }

            sortInternal(compareElements, lowIndex, partitionIndex - 1);
            sortInternal(compareElements, partitionIndex + 1, highIndex);
        }
    }
};

template<typename T>
Array<T> makeArray(s32 capacity, T* items, s32 count = 0)
{
    Array<T> result;
    result.capacity = capacity;
    result.count = count;
    result.items = items;

    return result;
}

template<typename T>
Array<T> makeEmptyArray()
{
    return makeArray<T>(0, nullptr);
}
