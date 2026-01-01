/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Colour.h"

Optional<Colour> Colour::read(LineReader& reader, LineReader::IsRequired is_required)
{
    // TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
    // We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

    auto all_arguments = reader.remainder_of_current_line();

    if (all_arguments.is_empty()) {
        if (is_required == LineReader::IsRequired::Yes)
            reader.error("Expected a colour."_s);
        return {};
    }

    auto r = reader.read_int<u8>();
    auto g = reader.read_int<u8>();
    auto b = reader.read_int<u8>();

    if (r.has_value() && g.has_value() && b.has_value()) {
        // NB: We default to fully opaque if no alpha is provided
        auto a = reader.read_int<u8>(LineReader::IsRequired::No);

        return Colour::from_rgb_255(r.release_value(), g.release_value(), b.release_value(), a.value_or(255));
    }

    reader.error("Couldn't parse '{0}' as a color. Expected 3 or 4 integers from 0 to 255, for R G B and optional A."_s, { all_arguments });
    return {};
}

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
