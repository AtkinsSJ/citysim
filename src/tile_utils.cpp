/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "tile_utils.h"
#include "building.h"
#include "render.h"
#include <Assets/AssetManager.h>

// The simplest possible algorithm is, just spread the 0s out that we marked above.
// (If a tile is not 0, set it to the min() of its 8 neighbours, plus 1.)
// We have to iterate through the area `maxDistance` times, but it should be fast enough probably.
void updateDistances(Array2<u8>* tileDistance, Rect2I dirtyRect, u8 maxDistance)
{
    DEBUG_FUNCTION();

    for (s32 iteration = 0; iteration < maxDistance; iteration++) {
        for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++) {
            for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++) {
                if (tileDistance->get(x, y) != 0) {
                    u8 minDistance = min(
                        tileDistance->getIfExists(x - 1, y - 1, 255),
                        tileDistance->getIfExists(x, y - 1, 255),
                        tileDistance->getIfExists(x + 1, y - 1, 255),
                        tileDistance->getIfExists(x - 1, y, 255),
                        //	tileDistance->getIfExists(x  , y  , 255),
                        tileDistance->getIfExists(x + 1, y, 255),
                        tileDistance->getIfExists(x - 1, y + 1, 255),
                        tileDistance->getIfExists(x, y + 1, 255),
                        tileDistance->getIfExists(x + 1, y + 1, 255));

                    if (minDistance != 255)
                        minDistance++;
                    if (minDistance > maxDistance)
                        minDistance = 255;

                    tileDistance->set(x, y, minDistance);
                }
            }
        }
    }
}

// @CopyPasta the other updateDistances().
void updateDistances(Array2<u8>* tileDistance, DirtyRects* dirtyRects, u8 maxDistance)
{
    DEBUG_FUNCTION();

    for (s32 iteration = 0; iteration < maxDistance; iteration++) {
        for (auto it = dirtyRects->rects.iterate();
            it.hasNext();
            it.next()) {
            Rect2I dirtyRect = it.getValue();

            for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++) {
                for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++) {
                    if (tileDistance->get(x, y) != 0) {
                        u8 minDistance = min(
                            tileDistance->getIfExists(x - 1, y - 1, 255),
                            tileDistance->getIfExists(x, y - 1, 255),
                            tileDistance->getIfExists(x + 1, y - 1, 255),
                            tileDistance->getIfExists(x - 1, y, 255),
                            //	tileDistance->getIfExists(x  , y  , 255),
                            tileDistance->getIfExists(x + 1, y, 255),
                            tileDistance->getIfExists(x - 1, y + 1, 255),
                            tileDistance->getIfExists(x, y + 1, 255),
                            tileDistance->getIfExists(x + 1, y + 1, 255));

                        if (minDistance != 255)
                            minDistance++;
                        if (minDistance > maxDistance)
                            minDistance = 255;

                        tileDistance->set(x, y, minDistance);
                    }
                }
            }
        }
    }
}
