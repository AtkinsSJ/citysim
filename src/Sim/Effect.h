/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Debug/Debug.h>
#include <IO/Forward.h>
#include <Util/Array2.h>
#include <Util/Basic.h>
#include <Util/Vector.h>

enum class EffectType : u8 {
    Add,
    Max,
};

class EffectRadius {
public:
    EffectRadius() = default;
    EffectRadius(s32 radius, s32 centre_value, s32 outer_value)
        : m_radius(radius)
        , m_centre_value(centre_value)
        , m_outer_value(outer_value)
    {
    }

    static Optional<EffectRadius> read(LineReader&);

    s32 radius() const { return m_radius; }
    bool has_effect() const { return m_radius > 0; }

    template<typename T>
    void apply(Array2<T>& tiles, Rect2I region, V2 effect_centre, EffectType type, float scale = 1.0f)
    {
        DEBUG_FUNCTION();

        float inverse_radius = 1.0f / m_radius;
        float square_radius = static_cast<float>(m_radius * m_radius);

        float centre_value = m_centre_value * scale;
        float outer_value = m_outer_value * scale;

        Rect2I possibleEffectArea { floor_s32(effect_centre.x - m_radius), floor_s32(effect_centre.y - m_radius), ceil_s32(m_radius + m_radius), ceil_s32(m_radius + m_radius) };
        possibleEffectArea = intersect(possibleEffectArea, region);
        for (s32 y = possibleEffectArea.y; y < possibleEffectArea.y + possibleEffectArea.h; y++) {
            for (s32 x = possibleEffectArea.x; x < possibleEffectArea.x + possibleEffectArea.w; x++) {
                float distance2FromSource = lengthSquaredOf(x - effect_centre.x, y - effect_centre.y);
                if (distance2FromSource <= square_radius) {
                    float contributionF = lerp(centre_value, outer_value, sqrt_float(distance2FromSource) * inverse_radius);
                    T contribution = static_cast<T>(floor_s32(contributionF));

                    if (contribution == 0)
                        continue;

                    switch (type) {
                    case EffectType::Add: {
                        T originalValue = tiles.get(x, y);

                        // This clamp is probably unnecessary but just in case.
                        T newValue = clamp<T>(originalValue + contribution, minPossibleValue<T>(), maxPossibleValue<T>());
                        tiles.set(x, y, newValue);
                    } break;

                    case EffectType::Max: {
                        T originalValue = tiles.get(x, y);
                        T newValue = max(originalValue, contribution);
                        tiles.set(x, y, newValue);
                    } break;

                        INVALID_DEFAULT_CASE;
                    }
                }
            }
        }
    }

private:
    s32 m_radius {};
    s32 m_centre_value {};
    s32 m_outer_value {};
};
