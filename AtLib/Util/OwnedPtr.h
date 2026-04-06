/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>

template<typename T>
class [[nodiscard]] OwnedPtr {
public:
    OwnedPtr() = default;

    OwnedPtr(decltype(nullptr))
        : m_ptr(nullptr)
    {
    }

    OwnedPtr(OwnedPtr&& other)
        : m_ptr(other.leak_ptr())
    {
    }

    OwnedPtr(OwnedRef<T>&& other)
        : m_ptr(other.leak_ptr())
    {
    }

    OwnedPtr& operator=(OwnedPtr&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    OwnedPtr& operator=(OwnedRef<T>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    OwnedPtr& operator=(OwnedPtr<U>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    OwnedPtr& operator=(OwnedRef<U>&& other)
    {
        clear();
        m_ptr = other.leak_ptr();
        return *this;
    }

    static OwnedPtr adopt(T* pointer)
    {
        return OwnedPtr { pointer };
    }

    // Not copyable
    OwnedPtr(OwnedPtr const&) = delete;
    OwnedPtr& operator=(OwnedPtr const&) = delete;

    ~OwnedPtr()
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

    bool operator==(std::nullptr_t const&) const
    {
        return m_ptr == nullptr;
    }

    void clear()
    {
        if (m_ptr)
            delete m_ptr;
        m_ptr = nullptr;
    }

    OwnedRef<T> release_nonnull()
    {
        ASSERT(m_ptr);
        OwnedRef<T> result { m_ptr };
        m_ptr = nullptr;
        return result;
    }

private:
    explicit OwnedPtr(T* ptr)
        : m_ptr(ptr)
    {
    }

    T* m_ptr { nullptr };
};

template<typename T>
class [[nodiscard]] OwnedRef {
public:
    friend class OwnedPtr<T>;
    enum class AdoptTag : u8 { Adopt };

    OwnedRef(OwnedRef&& other)
        : m_ptr(other.leak_ptr())
    {
        ASSERT(m_ptr);
    }

    template<typename U>
    OwnedRef(OwnedRef<U>&& other)
        : m_ptr(other.leak_ptr())
    {
        ASSERT(m_ptr);
    }

    OwnedRef& operator=(OwnedRef&& other)
    {
        m_ptr = other.leak_ptr();
        return *this;
    }

    template<typename U>
    OwnedRef& operator=(OwnedRef<U>&& other)
    {
        m_ptr = other.leak_ptr();
        return *this;
    }

    OwnedRef(AdoptTag, T& pointer)
        : m_ptr(&pointer)
    {
    }

    // Not copyable
    OwnedRef(OwnedRef const&) = delete;
    OwnedRef& operator=(OwnedRef const&) = delete;

    ~OwnedRef()
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

    operator T&() const
    {
        return this->operator*();
    }

    T* operator->() const
    {
        return m_ptr;
    }

    operator bool() const = delete;
    bool operator!() const = delete;

private:
    explicit OwnedRef(T* ptr)
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
OwnedPtr<T> adopt_own_if_nonnull(T* pointer)
{
    if (pointer)
        return OwnedPtr<T>::adopt(pointer);
    return {};
}

template<typename T>
OwnedRef<T> adopt_own(T& object)
{
    return OwnedRef<T> { OwnedRef<T>::AdoptTag::Adopt, object };
}
