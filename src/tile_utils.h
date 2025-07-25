/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "dirty.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <Util/Array2.h>
#include <Util/Basic.h>
#include <Util/Vector.h>

struct City;

struct EffectRadius {
    s32 centreValue;
    s32 radius;
    s32 outerValue;
};
inline bool hasEffect(EffectRadius* effectRadius) { return effectRadius->radius > 0; }

enum class EffectType : u8 {
    Add,
    Max,
};
template<typename T>
void applyEffect(EffectRadius* effectRadius, V2 effectCentre, EffectType type, Array2<T>* tileValues, Rect2I region, float scale = 1.0f)
{
    DEBUG_FUNCTION();

    float radius = (float)effectRadius->radius;
    float invRadius = 1.0f / radius;
    float radius2 = (float)(radius * radius);

    float centreValue = effectRadius->centreValue * scale;
    float outerValue = effectRadius->outerValue * scale;

    Rect2I possibleEffectArea = irectXYWH(floor_s32(effectCentre.x - radius), floor_s32(effectCentre.y - radius), ceil_s32(radius + radius), ceil_s32(radius + radius));
    possibleEffectArea = intersect(possibleEffectArea, region);
    for (s32 y = possibleEffectArea.y; y < possibleEffectArea.y + possibleEffectArea.h; y++) {
        for (s32 x = possibleEffectArea.x; x < possibleEffectArea.x + possibleEffectArea.w; x++) {
            float distance2FromSource = lengthSquaredOf(x - effectCentre.x, y - effectCentre.y);
            if (distance2FromSource <= radius2) {
                float contributionF = lerp(centreValue, outerValue, sqrt_float(distance2FromSource) * invRadius);
                T contribution = (T)floor_s32(contributionF);

                if (contribution == 0)
                    continue;

                switch (type) {
                case EffectType::Add: {
                    T originalValue = tileValues->get(x, y);

                    // This clamp is probably unnecessary but just in case.
                    T newValue = clamp<T>(originalValue + contribution, minPossibleValue<T>(), maxPossibleValue<T>());
                    tileValues->set(x, y, newValue);
                } break;

                case EffectType::Max: {
                    T originalValue = tileValues->get(x, y);
                    T newValue = max(originalValue, contribution);
                    tileValues->set(x, y, newValue);
                } break;

                    INVALID_DEFAULT_CASE;
                }
            }
        }
    }
}

// NB: This is a REALLY slow function! It's great for throwing in as a temporary solution, but
// if you want to use it a lot, then a per-tile distance array that's modified with updateDistances()
// when the map changes is much faster.
template<typename Filter>
s32 calculateDistanceTo(City* city, s32 x, s32 y, s32 maxDistanceToCheck, Filter filter)
{
    DEBUG_FUNCTION();

    s32 result = s32Max;

    if (filter(city, x, y)) {
        result = 0;
    } else {
        bool done = false;

        for (s32 distance = 1;
            !done && distance <= maxDistanceToCheck;
            distance++) {
            s32 leftX = x - distance;
            s32 rightX = x + distance;
            s32 bottomY = y - distance;
            s32 topY = y + distance;

            for (s32 px = leftX; px <= rightX; px++) {
                if (filter(city, px, bottomY)) {
                    result = distance;
                    done = true;
                    break;
                }

                if (filter(city, px, topY)) {
                    result = distance;
                    done = true;
                    break;
                }
            }

            if (done)
                break;

            for (s32 py = bottomY; py <= topY; py++) {
                if (filter(city, leftX, py)) {
                    result = distance;
                    done = true;
                    break;
                }

                if (filter(city, rightX, py)) {
                    result = distance;
                    done = true;
                    break;
                }
            }
        }
    }

    return result;
}

void updateDistances(Array2<u8>* tileDistance, Rect2I dirtyRect, u8 maxDistance);
void updateDistances(Array2<u8>* tileDistance, DirtyRects* dirtyRects, u8 maxDistance);
