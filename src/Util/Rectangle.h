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
public:
    Rect2I() = default;

    Rect2I(s32 x, s32 y, s32 w, s32 h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    template<typename U>
    Rect2I(U x, U y, U w, U h)
        : x(x)
        , y(y)
        , w(w)
        , h(h)
    {
    }

    Rect2I(V2I position, V2I size)
        : x(position.x)
        , y(position.y)
        , w(size.x)
        , h(size.y)
    {
    }

    static Rect2I create_centre_size(V2I centre, V2I size);
    static Rect2I create_centre_size(s32 centre_x, s32 centre_y, s32 w, s32 h);
    static Rect2I create_min_max(s32 min_x, s32 min_y, s32 max_x, s32 max_y);
    static Rect2I create_aligned(s32 origin_x, s32 origin_y, s32 w, s32 h, Alignment alignment);
    static Rect2I create_aligned(V2I origin, V2I size, Alignment alignment);

    // Rectangle that SHOULD contain anything. It is the maximum size, and centred
    // on 0,0 In practice, the distance between the minimum and maximum integers
    // is double what we can store, so the min and max possible positions are
    // outside this rectangle, by a long way (0.5 * s32Max). But I cannot conceive
    // that we will ever need to deal with positions that are anywhere near that,
    // so it should be fine.
    // - Sam, 08/02/2021
    static Rect2I create_infinity()
    {
        return { s32Min / 2, s32Min / 2, s32Max, s32Max };
    }

    // Rectangle that is guaranteed to not contain anything, because it's inside-out
    static Rect2I create_negative_infinity()
    {
        return { s32Max, s32Max, s32Min, s32Min };
    }

    s32 x;
    s32 y;
    s32 w;
    s32 h;
    static Rect2I placed_randomly_within(Random& random, V2I size, Rect2I boundary);

    V2I size() const { return v2i(w, h); }
    void set_size(V2I size)
    {
        w = size.x;
        h = size.y;
    }

    V2 centre() const;

    V2I position() const { return v2i(x, y); }
    void set_position(V2I position)
    {
        x = position.x;
        y = position.y;
    }

    s32 area() const;
    bool has_positive_area() const;
};

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

Rect2I centreWithin(Rect2I outer, V2I innerSize);
Rect2I alignWithinRectangle(Rect2I bounds, V2I size, Alignment alignment, Padding padding = {});
