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

class Rect2 {
public:
    Rect2() = default;

    Rect2(float x, float y, float w, float h)
        : m_x(x)
        , m_y(y)
        , m_width(w)
        , m_height(h)
    {
    }

    Rect2(s32 x, s32 y, s32 w, s32 h)
        : m_x(x)
        , m_y(y)
        , m_width(w)
        , m_height(h)
    {
    }

    Rect2(V2 pos, V2 size)
        : m_x(pos.x)
        , m_y(pos.y)
        , m_width(size.x)
        , m_height(size.y)
    {
    }

    Rect2(Rect2I const& other);

    static Rect2 create_min_max(float min_x, float min_y, float max_x, float max_y);
    static Rect2 create_aligned(V2 origin, V2 size, Alignment alignment);

    float x() const { return m_x; }
    void set_x(float x) { m_x = x; }
    float y() const { return m_y; }
    void set_y(float y) { m_y = y; }

    float min_x() const { return m_x; }
    float max_x() const { return m_x + m_width; }
    float min_y() const { return m_y; }
    float max_y() const { return m_y + m_height; }

    float width() const { return m_width; }
    void set_width(float width) { m_width = width; }

    float height() const { return m_height; }
    void set_height(float height) { m_height = height; }

    V2 size() const { return v2(m_width, m_height); }
    void set_size(V2 size);

    V2 centre() const;

    V2 position() const { return v2(m_x, m_y); }
    void set_position(V2 position);

    void translate(V2 offset);

    float area() const;
    bool has_positive_area() const;

    bool contains(float x, float y) const;
    bool contains(V2) const;
    bool contains(Rect2 const&) const;

    bool overlaps(Rect2 const&) const;

    Rect2 translated(V2 offset) const;
    Rect2 expanded(float radius) const;
    Rect2 expanded(float top, float right, float bottom, float left) const;
    Rect2 intersected(Rect2 const&) const;
    Rect2 intersected_relative(Rect2 const&) const;

private:
    float m_x { 0 };
    float m_y { 0 };
    float m_width { 0 };
    float m_height { 0 };
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
