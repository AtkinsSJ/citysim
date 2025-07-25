/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Colour.h"

Colour Colour::as_opaque() const
{
    // Colors are always stored with premultiplied alpha, so in order to set the alpha
    // back to 100%, we have to divide by that alpha.

    // If alpha is 0, we can't divide, so just set it
    // If alpha is already 1, we don't need to do anything
    // If alpha is > 1, clamp it
    if (m_a == 0.0f || m_a >= 1.0f)
        return { m_r, m_g, m_b, 1.0f };

    // Otherwise, divide by the alpha and then make it 1
    return {
        m_r / m_a,
        m_g / m_a,
        m_b / m_a,
        1.0f,
    };
}

Colour Colour::multiplied_by(Colour const& other) const
{
    return {
        m_r * other.m_r,
        m_g * other.m_g,
        m_b * other.m_b,
        m_a * other.m_a,
    };
}
