/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Vector.h>

class Colour {
public:
    Colour() = default;
    Colour(float r, float g, float b, float a)
        : m_r(r)
        , m_g(g)
        , m_b(b)
        , m_a(a)
    {
    }

    // FIXME: Temporary conversion
    Colour(V4 source)
        : m_r(source.r)
        , m_g(source.g)
        , m_b(source.b)
        , m_a(source.a)
    {
    }

    operator V4() const
    {
        return v4(m_r, m_g, m_b, m_a);
    }

    static Colour from_rgb_255(u8 r, u8 g, u8 b, u8 a)
    {
        float constexpr inverse_255 = 1.0f / 255.0f;
        float alpha = a * inverse_255;

        return Colour {
            r * inverse_255 * alpha,
            g * inverse_255 * alpha,
            b * inverse_255 * alpha,
            alpha,
        };
    }

    static Colour white()
    {
        return { 1, 1, 1, 1 };
    }

    float r() const { return m_r; }
    float g() const { return m_g; }
    float b() const { return m_b; }
    float a() const { return m_a; }

    Colour as_opaque() const;
    Colour multiplied_by(Colour const&) const;

    Colour operator*(float value) const
    {
        return {
            m_r * value,
            m_g * value,
            m_b * value,
            m_a * value,
        };
    }

private:
    float m_r { 0 };
    float m_g { 0 };
    float m_b { 0 };
    float m_a { 0 };
};

inline Colour lerp(Colour const& a, Colour const& b, f32 position)
{
    return Colour {
        (a.r() + ((b.r() - a.r()) * position)),
        (a.g() + ((b.g() - a.g()) * position)),
        (a.b() + ((b.b() - a.b()) * position)),
        (a.a() + ((b.a() - a.a()) * position)),
    };
}
