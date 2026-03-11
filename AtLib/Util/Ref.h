/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

template<typename T>
class Ref {
public:
    Ref() = delete;

    Ref(T& value)
        : m_value(&value)
    {
    }

    operator T&() const { return *m_value; }
    T* operator->() const { return m_value; }
    T& operator*() const { return *m_value; }

    bool operator==(Ref const&) const = default;

private:
    T* m_value;
};
