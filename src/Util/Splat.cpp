/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Splat.h"
#include <Util/Random.h>

Splat Splat::create_random(s32 centreX, s32 centreY, float min_radius, float max_radius, s32 resolution, Random* random, s32 smoothness, MemoryArena* memoryArena)
{
    auto radius = memoryArena->allocate_array<float>(resolution, true);
    random->fill_with_noise(radius, smoothness, true);

    return Splat { v2i(centreX, centreY), min_radius, max_radius, move(radius), resolution / 360.0f };
}

float Splat::radius_at_angle(float degrees) const
{
    // Interpolate between the two nearest radius values
    float desired_index = degrees * m_degrees_per_index;
    s32 indexA = (floor_s32(desired_index) + m_radius.count) % m_radius.count;
    s32 indexB = (ceil_s32(desired_index) + m_radius.count) % m_radius.count;
    float noiseAtAngle = lerp(m_radius[indexA], m_radius[indexB], fraction_float(desired_index));

    return m_min_radius + (noiseAtAngle * (m_max_radius - m_min_radius));
}

Rect2I Splat::bounding_box() const
{
    s32 diameter = 1 + ceil_s32(m_max_radius * 2.0f);
    return Rect2I::create_centre_size(m_centre.x, m_centre.y, diameter, diameter);
}

bool Splat::contains(s32 x, s32 y) const
{
    float distance = lengthOf(x - m_centre.x, y - m_centre.y);

    if (distance > m_max_radius) {
        // If we're outside the max, we must be outside the blob
        return false;
    }
    if (distance <= m_min_radius) {
        // If we're inside the min, we MUST be in the blob
        return true;
    }

    float angle = angleOf(x - m_centre.x, y - m_centre.y);
    return distance < radius_at_angle(angle);
}
