/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>

template<typename T>
class [[nodiscard]] OwnPtr {
public:
    OwnPtr() = default;

    OwnPtr(decltype(nullptr))
        : m_ptr(nullptr)
    {
    }

    OwnPtr(OwnPtr&& other)
        : m_ptr(other.leak_ptr())
    {
    }

    OwnPtr(NonnullOwnPtr<T>&& other)
        : m_ptr(other.leak_ptr())
    {
    }

    OwnPtr& operator=(OwnPtr&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    OwnPtr& operator=(NonnullOwnPtr<T>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    OwnPtr& operator=(OwnPtr<U>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    OwnPtr& operator=(NonnullOwnPtr<U>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    static OwnPtr adopt(T* pointer)
    {
        return OwnPtr { pointer };
    }

    // Not copyable
    OwnPtr(OwnPtr const&) = delete;
    OwnPtr& operator=(OwnPtr const&) = delete;

    ~OwnPtr()
    {
        if (m_ptr)
            clear();
    }

    T* ptr() const
    {
        return m_ptr;
    }

    T* leak_ptr()
    {
        T* result = move(m_ptr);
        m_ptr = nullptr;
        return result;
    }

    T& operator*() const
    {
        ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        ASSERT(m_ptr);
        return m_ptr;
    }

    operator bool() const
    {
        return m_ptr != nullptr;
    }

    bool operator!() const
    {
        return m_ptr == nullptr;
    }

    void clear()
    {
        if (m_ptr)
            delete m_ptr;
        m_ptr = nullptr;
    }

    NonnullOwnPtr<T> release_nonnull()
    {
        ASSERT(m_ptr);
        NonnullOwnPtr<T> result { m_ptr };
        m_ptr = nullptr;
        return result;
    }

private:
    explicit OwnPtr(T* ptr)
        : m_ptr(ptr)
    {
    }

    T* m_ptr { nullptr };
};

template<typename T>
class [[nodiscard]] NonnullOwnPtr {
public:
    friend class OwnPtr<T>;
    enum class AdoptTag : u8 { Adopt };

    NonnullOwnPtr(NonnullOwnPtr&& other)
        : m_ptr(other.m_ptr)
    {
        ASSERT(m_ptr);
        other.m_ptr = nullptr;
    }

    NonnullOwnPtr& operator=(NonnullOwnPtr&& other)
    {
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    NonnullOwnPtr& operator=(NonnullOwnPtr<U>&& other)
    {
        m_ptr = other.leak_ptr();
        return *this;
    }

    NonnullOwnPtr(AdoptTag, T& pointer)
        : m_ptr(&pointer)
    {
    }

    // Not copyable
    NonnullOwnPtr(NonnullOwnPtr const&) = delete;
    NonnullOwnPtr& operator=(NonnullOwnPtr const&) = delete;

    ~NonnullOwnPtr()
    {
        clear();
    }

    T* ptr() const
    {
        return m_ptr;
    }

    T* leak_ptr()
    {
        ASSERT(m_ptr);
        T* result = move(m_ptr);
        m_ptr = nullptr;
        return result;
    }

    T& operator*() const
    {
        ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        return m_ptr;
    }

    operator bool() const = delete;
    bool operator!() const = delete;

private:
    explicit NonnullOwnPtr(T* ptr)
        : m_ptr(ptr)
    {
    }

    void clear()
    {
        delete m_ptr;
        m_ptr = nullptr;
    }

    T* m_ptr { nullptr };
};

template<typename T>
OwnPtr<T> adopt_own_if_nonnull(T* pointer)
{
    if (pointer)
        return OwnPtr<T>::adopt(pointer);
    return {};
}

template<typename T>
NonnullOwnPtr<T> adopt_own(T& object)
{
    return NonnullOwnPtr<T> { NonnullOwnPtr<T>::AdoptTag::Adopt, object };
}
