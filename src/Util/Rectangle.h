/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Alignment.h>
#include <Util/Basic.h>
#include <Util/Padding.h>
#include <Util/Vector.h>

struct Rect2 {
    Rect2() = default;

    Rect2(float x, float y, float w, float h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    Rect2(s32 x, s32 y, s32 w, s32 h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    Rect2(V2 pos, V2 size)
        : x(pos.x)
        , y(pos.y)
        , w(size.x)
        , h(size.y)
    {
    }

    Rect2(Rect2I const& other);

    static Rect2 create_min_max(float minX, float minY, float maxX, float maxY);
    static Rect2 create_aligned(V2 origin, V2 size, Alignment alignment);

    union {
        V2 pos;
        struct
        {
            float x { 0 };
            float y { 0 };
        };
    };
    union {
        V2 size;
        struct
        {
            float w { 0 };
            float h { 0 };
        };
    };

    float left() const { return x; }
    float right() const { return x + w; }
    float top() const { return y; }
    float bottom() const { return y + h; }

    V2 centre() const;

    float area() const;
    bool has_positive_area() const;

    bool contains(float x, float y) const;
    bool contains(V2) const;
    bool contains(Rect2 const&) const;

    bool overlaps(Rect2 const&) const;

    Rect2 expanded(float radius) const;
    Rect2 expanded(float top, float right, float bottom, float left) const;
    Rect2 intersected(Rect2 const&) const;
    Rect2 intersected_relative(Rect2 const&) const;
};

struct Rect2I {
    union {
        V2I pos;
        struct
        {
            s32 x;
            s32 y;
        };
    };
    union {
        V2I size;
        struct
        {
            s32 w;
            s32 h;
        };
    };

    static Rect2I placed_randomly_within(Random& random, V2I size, Rect2I boundary);
};

Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h);
Rect2I irectInfinity();         // Contains approximately everything...
Rect2I irectNegativeInfinity(); // Contains nothing
Rect2I irectPosSize(V2I position, V2I size);
Rect2I irectCentreSize(s32 centreX, s32 centreY, s32 sizeX, s32 sizeY);
Rect2I irectCentreSize(V2I position, V2I size);
Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax);
Rect2I irectAligned(s32 originX, s32 originY, s32 w, s32 h, Alignment alignment);
Rect2I irectAligned(V2I origin, V2I size, Alignment alignment);

bool contains(Rect2I rect, s32 x, s32 y);
bool contains(Rect2I rect, V2I pos);
bool contains(Rect2I rect, V2 pos);
bool contains(Rect2I outer, Rect2I inner);
bool overlaps(Rect2I outer, Rect2I inner);

Rect2I expand(Rect2I rect, s32 radius);
Rect2I expand(Rect2I rect, s32 top, s32 right, s32 bottom, s32 left);
// NB: shrink() always keeps the rectangle at >=0 sizes, so if you attempt
// to subtract more, it remains 0 wide or tall, with a correct centre.
// (This means shrinking a 0-size rectangle asymmetrically will move the
// rectangle around. Which isn't useful so I don't know why I'm documenting it.)
Rect2I shrink(Rect2I rect, s32 radius);
Rect2I shrink(Rect2I rect, Padding padding);
Rect2I intersect(Rect2I a, Rect2I b);
// Takes the intersection of inner and outer, and then converts it to being relative to outer.
// (Originally used to take a world-space rectangle and put it into a cropped, sector-space one.)
Rect2I intersectRelative(Rect2I outer, Rect2I inner);
Rect2I unionOf(Rect2I a, Rect2I b);

V2 centreOf(Rect2I rect);
V2I centreOfI(Rect2I rect);
s32 areaOf(Rect2I rect); // Always positive, even if the rect has negative dimensions
bool hasPositiveArea(Rect2I rect);

Rect2I centreWithin(Rect2I outer, V2I innerSize);
Rect2I alignWithinRectangle(Rect2I bounds, V2I size, Alignment alignment, Padding padding = {});
