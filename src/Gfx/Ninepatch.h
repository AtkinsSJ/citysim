/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Util/Basic.h>

struct Ninepatch {
    AssetMetadata* texture;

    s32 pu0;
    s32 pu1;
    s32 pu2;
    s32 pu3;

    s32 pv0;
    s32 pv1;
    s32 pv2;
    s32 pv3;

    // NB: These UV values do not change once they're set in loadAsset(), so, we
    // could replace them with an array of 9 Rect2s in order to avoid calculating
    // them over and over. BUT, this would greatly increase the size of the Asset
    // struct itself, which we don't want to do. We could move that into the
    // Asset.data blob, but then we're doing an extra indirection every time we
    // read it, and it's not *quite* big enough to warrant that extra complexity.
    // Frustrating! If it was bigger I'd do it, but as it is, I think keeping
    // these 8 floats is the best option. If creating the UV rects every time
    // becomes a bottleneck, we can switch over.
    // - Sam, 16/02/2021
    float u0;
    float u1;
    float u2;
    float u3;

    float v0;
    float v1;
    float v2;
    float v3;
};
