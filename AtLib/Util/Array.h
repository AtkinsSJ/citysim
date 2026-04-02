/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Badge.h>
#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Span.h>

template<typename T>
struct Array {
    Array() = default;
    Array(size_t capacity, T* items, size_t count = 0)
        : capacity(capacity)
        , count(count)
        , items(items)
    {
    }

    size_t capacity { 0 };
    size_t count { 0 };
    T* items { nullptr };

    bool is_empty() const
    {
        return count == 0;
    }

    T& operator[](s32 index)
    {
        ASSERT(index >= 0 && index < count); // Index out of range!
        return items[index];
    }
    T const& operator[](s32 index) const { return const_cast<Array&>(*this)[index]; }

    // Same as [] but easier when we're using an Array*
    T& get(s32 index)
    {
        ASSERT(index >= 0 && index < count); // Index out of range!
        return items[index];
    }
    T const& get(s32 index) const { return const_cast<Array&>(*this).get(index); }

    T& first()
    {
        ASSERT(count > 0); // Index out of range!
        return items[0];
    }
    T const& first() const { return const_cast<Array&>(*this).first(); }

    T& last()
    {
        ASSERT(count > 0); // Index out of range!
        return items[count - 1];
    }
    T const& last() const { return const_cast<Array&>(*this).last(); }

    T* append()
    {
        ASSERT(count < capacity);
        return new (&items[count++]) T();
    }

    T* append(T const& item)
    {
        ASSERT(count < capacity);
        return new (&items[count++]) T(item);
    }

    T* append(T&& item)
    {
        ASSERT(count < capacity);
        return new (&items[count++]) T(move(item));
    }

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

    Span<T> span()
    {
        return { count, items };
    }

    ReadonlySpan<T> span() const
    {
        return { count, items };
    }

    operator Span<T>() { return span(); }
    operator ReadonlySpan<T>() const { return span(); }

    class Iterator {
    public:
        explicit Iterator(Badge<Array<T>>, T* pointer)
            : m_pointer(pointer)
        {
        }

        T& operator*() { return *m_pointer; }
        T* operator->() { return m_pointer; }

        Iterator& operator++()
        {
            ++m_pointer;
            return *this;
        }

        Iterator operator++(int)
        {
            auto result = *this;
            ++(*this);
            return result;
        }

        bool operator==(Iterator const& other)
        {
            return m_pointer == other.m_pointer;
        }

        bool operator!=(Iterator const& other)
        {
            return m_pointer != other.m_pointer;
        }

    private:
        T* m_pointer;
    };

    Iterator begin() const
    {
        return Iterator({}, items);
    }

    Iterator end() const
    {
        return Iterator({}, items + count);
    }
};
