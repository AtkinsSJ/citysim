/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Maths.h>
#include <typeinfo>

namespace Detail {

template<typename... Fs>
struct Visitor : Fs... {
    Visitor(Fs&&... functions)
        : Fs(functions)...
    {
    }

    using Fs::operator()...;
};

template<typename T, typename... Rest>
class VisitHelper {
public:
    template<typename... Fs>
    static decltype(auto) visit(std::type_info const* type, u8 const* data, Visitor<Fs...> visitor)
    {
        if (type == &typeid(T))
            return visitor(*reinterpret_cast<T*>(const_cast<u8*>(data)));
        if constexpr (sizeof...(Rest) > 0)
            return VisitHelper<Rest...>::visit(type, data, forward<Visitor<Fs...>>(visitor));
        else
            VERIFY_NOT_REACHED();
    }

    template<typename... Fs>
    static decltype(auto) visit_const(std::type_info const* type, u8 const* data, Visitor<Fs...> visitor)
    {
        if (type == &typeid(T))
            return visitor(*reinterpret_cast<T const*>(data));
        if constexpr (sizeof...(Rest) > 0)
            return VisitHelper<Rest...>::visit(type, data, forward<Visitor<Fs...>>(visitor));
        else
            VERIFY_NOT_REACHED();
    }
};

}

struct Empty {
    constexpr bool operator==(Empty const&) const = default;
};

template<typename... Ts>
class Variant {
public:
    friend class VisitHelper;

    template<typename T>
    static constexpr bool can_contain()
    {
        return (std::is_same_v<T, Ts> || ...);
    }

    template<typename T>
    Variant(T&& t)
    requires(can_contain<T>())
    {
        set_without_destructing_old_value(forward<T>(t));
    }

    Variant()
    requires(can_contain<Empty>())
        : Variant(Empty {})
    {
    }

    Variant()
    requires(!can_contain<Empty>())
    = delete;

    Variant(Variant&& other)
    {
        other.visit([&](auto& it) {
            set_without_destructing_old_value(move(it));
        });
    }

    Variant(Variant const& other)
    {
        other.visit([&](auto& it) {
            set_without_destructing_old_value(it);
        });
    }

    ~Variant()
    {
        destruct_value();
    }

    template<typename T>
    bool has() const
    requires(can_contain<T>())
    {
        return m_current_type == &typeid(T);
    }

    template<typename T>
    T& get()
    requires(can_contain<T>())
    {
        ASSERT(has<T>());
        return *reinterpret_cast<T*>(m_data);
    }

    template<typename T>
    T const& get() const
    requires(can_contain<T>())
    {
        ASSERT(has<T>());
        return *reinterpret_cast<T const*>(m_data);
    }

    template<typename T>
    T* try_get()
    requires(can_contain<T>())
    {
        if (has<T>())
            return reinterpret_cast<T*>(m_data);
        return nullptr;
    }

    template<typename T>
    T const* try_get() const
    requires(can_contain<T>())
    {
        if (has<T>())
            return reinterpret_cast<T const*>(m_data);
        return nullptr;
    }

    template<typename T>
    void set(T&& t)
    requires(can_contain<T>())
    {
        destruct_value();
        set_without_destructing_old_value(forward<T>(t));
    }

    template<typename T>
    Variant& operator=(T&& t)
    requires(can_contain<T>())
    {
        set(forward<T>(t));
        return *this;
    }

    bool operator==(Variant const& other) const
    {
        if (m_current_type != other.m_current_type)
            return false;

        return visit([&other]<typename T>(T const& t) {
            if (auto* other_t = other.try_get<T>()) {
                return *other_t == t;
            }
            return false;
        });
    }

    template<typename... Fs>
    decltype(auto) visit(Fs&&... functions)
    {
        Detail::Visitor visitor { forward<Fs>(functions)... };
        Detail::VisitHelper<Ts...> helper;
        return helper.visit(m_current_type, m_data, visitor);
    }

    template<typename... Fs>
    decltype(auto) visit(Fs&&... functions) const
    {
        Detail::Visitor visitor { forward<Fs>(functions)... };
        Detail::VisitHelper<Ts...> helper;
        return helper.visit_const(m_current_type, m_data, visitor);
    }

private:
    static constexpr size_t data_size = max(sizeof(Ts)...);

    template<typename T>
    void set_without_destructing_old_value(T&& t)
    requires(can_contain<T>())
    {
        new (m_data) T(forward<T>(t));
        m_current_type = &typeid(T);
    }

    template<typename T>
    bool destruct_value_of_type()
    {
        if (has<T>()) {
            reinterpret_cast<T*>(m_data)->~T();
            return true;
        }
        return false;
    }

    void destruct_value()
    {
        (destruct_value_of_type<Ts>() || ...);
    }

    alignas(Ts...) u8 m_data[data_size];
    std::type_info const* m_current_type;
};
