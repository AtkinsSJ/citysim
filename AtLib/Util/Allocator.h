/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Array2.h>
#include <Util/Span.h>

class Allocator {
public:
    virtual ~Allocator() = default;

    template<typename T, typename... Args>
    T* allocate(Args&&... args)
    {
        auto memory = allocate_internal(sizeof(T));
        return new (memory.raw_data()) T(forward<Args>(args)...);
    }

    template<typename T>
    Span<T> allocate_multiple(size_t count)
    {
        if (count == 0)
            return {};

        auto memory = allocate_internal(sizeof(T) * count);
        return Span { count, reinterpret_cast<T*>(memory.raw_data()) };
    }

    template<typename T, typename Item = u8>
    struct ObjectAndData {
        T& object;
        Span<Item> data;
    };
    template<typename T, typename Item = u8>
    ObjectAndData<T, Item> allocate_with_data(size_t item_count)
    {
        // FIXME: Figure out alignment requirements so that the Item array is aligned too.
        auto memory = allocate_internal(sizeof(T) + (sizeof(Item) * item_count));
        ObjectAndData<T, Item> result {
            .object = *reinterpret_cast<T*>(memory.raw_data()),
            .data = { item_count, reinterpret_cast<Item*>(reinterpret_cast<u8*>(memory.raw_data()) + sizeof(T)) },
        };
        new (&result.object) T();
        return result;
    }

    // FIXME: Maybe these two should be different types?
    template<typename T>
    Array<T> allocate_array(size_t count)
    {
        auto items = allocate_multiple<T>(count);
        return Array<T> { items.size(), items.raw_data(), 0 };
    }
    template<typename T>
    Array<T> allocate_filled_array(size_t count, T const& value = {})
    {
        auto items = allocate_multiple<T>(count);
        new (items.raw_data()) T[count](value);
        return Array<T> { count, items.raw_data(), count };
    }

    template<typename T>
    Array2<T> allocate_array_2d(V2I size)
    {
        return allocate_array_2d<T>(size.x, size.y);
    }

    template<typename T>
    Array2<T> allocate_array_2d(u32 w, u32 h)
    {
        ASSERT(w > 0 && h > 0);
        auto items = allocate_multiple<T>(w * h);
        new (items.raw_data()) T[w * h];
        return Array2<T> { w, h, items };
    }

    String allocate_string(StringView input);
    Blob allocate_blob(size_t size);
    Blob allocate_blob(Blob const& source);

    void deallocate(Blob&);
    void deallocate(String&);

    template<typename T>
    void deallocate(Array<T>& array)
    {
        array.~Array();
        deallocate_internal({ array.capacity() * sizeof(T), reinterpret_cast<u8*>(array.raw_items()) });
        array = {};
    }

protected:
    // FIXME: Handle alignment here
    virtual Span<u8> allocate_internal(size_t size) = 0;
    virtual void deallocate_internal(Span<u8>) = 0;
};
