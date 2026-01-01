/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Concepts.h>
#include <Util/OwnPtr.h>

template<typename>
class Function;

template<typename Out, typename... In>
class Function<Out(In...)> {
public:
    Function() = default;
    Function(std::nullptr_t)
        : Function()
    {
    }

    template<typename Callable>
    requires(IsCallableWithArguments<Callable, Out, In...>)
    Function(Callable&& callable)
    {
        m_callable = adopt_own_if_nonnull(new Wrapper<Callable> { move(callable) });
    }

    template<typename Callable>
    requires(IsCallableWithArguments<Callable, Out, In...>)
    Function& operator=(Callable&& callable)
    {
        m_callable = adopt_own_if_nonnull(new Wrapper<Callable> { move(callable) });
        return *this;
    }

    Out operator()(In... in)
    {
        ASSERT(m_callable);
        return m_callable->call(forward<In>(in)...);
    }

    operator bool()
    {
        return m_callable;
    }

private:
    class WrapperBase {
    public:
        virtual ~WrapperBase() = default;
        virtual Out call(In...) = 0;
    };

    template<typename Callable>
    class Wrapper : public WrapperBase {
    public:
        explicit Wrapper(Callable&& callable)
            : m_callable(move(callable))
        {
        }

        virtual ~Wrapper() override = default;
        virtual Out call(In... in) override
        {
            return m_callable.operator()(forward<In>(in)...);
        }

    private:
        Callable m_callable;
    };
    OwnPtr<WrapperBase> m_callable;
};
