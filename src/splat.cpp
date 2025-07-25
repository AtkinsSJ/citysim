/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "splat.h"
#include <Util/Random.h>

Splat createRandomSplat(s32 centreX, s32 centreY, float minRadius, float maxRadius, s32 resolution, Random* random, s32 smoothness, MemoryArena* memoryArena)
{
    Splat result;

    result.centre = v2i(centreX, centreY);
    result.minRadius = minRadius;
    result.maxRadius = maxRadius;
    result.radiusRange = maxRadius - minRadius;

    result.radius = memoryArena->allocate_array<float>(resolution, true);
    result.degreesToIndex = resolution / 360.0f;

    generate1DNoise(random, &result.radius, smoothness, true);

    return result;
}

float getRadiusAtAngle(Splat* splat, float degrees)
{
    // Interpolate between the two nearest radius values
    float desiredIndex = degrees * splat->degreesToIndex;
    s32 indexA = (floor_s32(desiredIndex) + splat->radius.count) % splat->radius.count;
    s32 indexB = (ceil_s32(desiredIndex) + splat->radius.count) % splat->radius.count;
    float noiseAtAngle = lerp(splat->radius[indexA], splat->radius[indexB], fraction_float(desiredIndex));

    float result = splat->minRadius + (noiseAtAngle * splat->radiusRange);
    return result;
}

Rect2I getBoundingBox(Splat* splat)
{
    s32 diameter = 1 + ceil_s32(splat->maxRadius * 2.0f);
    return irectCentreSize(splat->centre.x, splat->centre.y, diameter, diameter);
}

bool contains(Splat* splat, s32 x, s32 y)
{
    bool result = false;

    float distance = lengthOf(x - splat->centre.x, y - splat->centre.y);

    if (distance > splat->maxRadius) {
        // If we're outside the max, we must be outside the blob
        result = false;
    } else if (distance <= splat->minRadius) {
        // If we're inside the min, we MUST be in the blob
        result = true;
    } else {
        float angle = angleOf(x - splat->centre.x, y - splat->centre.y);

        float radiusAtAngle = getRadiusAtAngle(splat, angle);

        if (distance < radiusAtAngle) {
            result = true;
        }
    }

    return result;
}
