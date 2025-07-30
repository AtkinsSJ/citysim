/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Util/Array2.h>
#include <Util/Basic.h>

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
