/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/ChunkedArray.h>
#include <Util/Forward.h>

//
// A stack! What more is there to say?
//
// For now (04/02/2021) this just wraps a ChunkedArray and limits what you can do with it.
// That saves a lot of effort and duplication. But it's a bit inelegant, so maybe this should
// be rewritten later. IDK! We basically are only making this a thing, instead of using a
// ChunkedArray directly, because we don't want user code to accidentally do operations on
// it that might leave a gap, as that would break the stack logic completely. Well, rather,
// it would make the stack logic a lot more complicated to check for gaps.
//

template<typename T>
class Stack {
public:
    Stack() = default; // FIXME: Temporary
    explicit Stack(MemoryArena& arena, s32 items_per_chunk = 32)
        : m_array(arena, items_per_chunk)
    {
    }

    bool is_empty() const { return m_array.is_empty(); }

    T* push(T item)
    {
        return m_array.append(item);
    }

    T* peek()
    {
        if (is_empty())
            return nullptr;

        return &m_array.get(m_array.count - 1);
    }

    T const* peek() const
    {
        return const_cast<Stack*>(this)->peek();
    }

    Optional<T> pop()
    {
        if (is_empty())
            return {};

        return m_array.take_index(m_array.count - 1);
    }

private:
    ChunkedArray<T> m_array;
};
