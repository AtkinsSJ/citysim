/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>
#include <Util/Variant.h>

template<typename T>
class [[nodiscard]] ErrorOr {
public:
    ErrorOr(Error&& error)
        : m_value(move(error))
    {
    }

    ErrorOr(T&& value)
        : m_value(move(value))
    {
    }

    bool is_error() const { return m_value.template has<Error>(); }
    Error const& error() const { return m_value.template get<Error>(); }

    T const& value() const { return m_value.template get<T>(); }
    T& value() { return m_value.template get<T>(); }

    T&& release_value() { return move(value()); }

private:
    Variant<Error, T> m_value;
};
