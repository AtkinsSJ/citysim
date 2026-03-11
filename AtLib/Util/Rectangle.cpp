/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Rectangle.h"
#include <Util/Maths.h>
#include <Util/Random.h>

/**********************************************
    Rect2
 **********************************************/

Rect2::Rect2(Rect2I const& other)
    : m_x(other.x())
    , m_y(other.y())
    , m_width(other.width())
    , m_height(other.height())
{
}

Rect2 Rect2::create_min_max(float min_x, float min_y, float max_x, float max_y)
{
    return {
        min_x,
        min_y,
        max_x - min_x,
        max_y - min_y,
    };
}

Rect2 Rect2::create_aligned(V2 origin, V2 size, Alignment alignment)
{
    float x = 0;
    float y = 0;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        x = origin.x - round_float(size.x / 2.0f);
        break;
    case HAlign::Right:
        x = origin.x - size.x;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Same as left
    default:
        x = origin.x;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        y = origin.y - round_float(size.y / 2.0f);
        break;
    case VAlign::Bottom:
        y = origin.y - size.y;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Same as top
    default:
        y = origin.y;
        break;
    }

    return { x, y, size.x, size.y };
}

bool Rect2::contains(float x, float y) const
{
    return min_x() <= x
        && x < max_x()
        && min_y() <= y
        && y < max_y();
}

bool Rect2::contains(V2 pos) const
{
    return contains(pos.x, pos.y);
}

bool Rect2::contains(Rect2 const& inner) const
{
    return contains(inner.position()) && contains(inner.position() + inner.size());
}

bool Rect2::overlaps(Rect2 const& b) const
{
    return min_x() < b.max_x()
        && b.min_x() < max_x()
        && min_y() < b.max_y()
        && b.min_y() < max_y();
}

Rect2 Rect2::translated(V2 offset) const
{
    return { m_x + offset.x, m_y + offset.y, m_width, m_height };
}

Rect2 Rect2::expanded(float radius) const
{
    return {
        m_x - radius,
        m_y - radius,
        m_width + (radius * 2.0f),
        m_height + (radius * 2.0f)
    };
}

Rect2 Rect2::expanded(float top, float right, float bottom, float left) const
{
    return {
        m_x - left,
        m_y - top,
        m_width + left + right,
        m_height + top + bottom
    };
}

Rect2 Rect2::intersected(Rect2 const& other) const
{
    Rect2 result = *this;

    // X
    float right_extension = (result.m_x + result.m_width) - (other.m_x + other.m_width);
    if (right_extension > 0) {
        result.m_width -= right_extension;
    }
    if (result.m_x < other.m_x) {
        float left_extension = other.m_x - result.m_x;
        result.m_x += left_extension;
        result.m_width -= left_extension;
    }

    // Y
    float bottom_extension = (result.m_y + result.m_height) - (other.m_y + other.m_height);
    if (bottom_extension > 0) {
        result.m_height -= bottom_extension;
    }
    if (result.m_y < other.m_y) {
        float top_extension = other.m_y - result.m_y;
        result.m_y += top_extension;
        result.m_height -= top_extension;
    }

    result.m_width = max(result.m_width, 0.0f);
    result.m_height = max(result.m_height, 0.0f);

    return result;
}

// Takes the intersection of inner and outer, and then converts it to being relative to outer.
// (Originally used to take a world-space rectangle and put it into a cropped, sector-space one.)
Rect2 Rect2::intersected_relative(Rect2 const& inner) const
{
    return intersected(inner).translated(-position());
}

V2 Rect2::centre() const
{
    return v2(
        m_x + (m_width / 2.0f),
        m_y + (m_height / 2.0f));
}

void Rect2::set_size(V2 size)
{
    m_width = size.x;
    m_height = size.y;
}

void Rect2::set_position(V2 position)
{
    m_x = position.x;
    m_y = position.y;
}

void Rect2::translate(V2 offset)
{
    m_x += offset.x;
    m_y += offset.y;
}

float Rect2::area() const
{
    return abs_float(m_width * m_height);
}

bool Rect2::has_positive_area() const
{
    return m_width > 0 && m_height > 0;
}

/**********************************************
        Rect2I
 **********************************************/

Rect2I Rect2I::placed_randomly_within(Random& random, V2I size, Rect2I boundary)
{
    return {
        random.random_between(boundary.m_x, boundary.m_x + boundary.m_width - size.x),
        random.random_between(boundary.m_y, boundary.m_y + boundary.m_height - size.y),
        size.x, size.y
    };
}

Rect2I Rect2I::create_centre_size(s32 centre_x, s32 centre_y, s32 w, s32 h)
{
    return { centre_x - (w / 2), centre_y - (h / 2), w, h };
}

Rect2I Rect2I::create_centre_size(V2I position, V2I size)
{
    return { position - (size / 2), size };
}

Rect2I Rect2I::create_min_max(s32 min_x, s32 min_y, s32 max_x, s32 max_y)
{
    return { min_x, min_y, (1 + max_x - min_x), (1 + max_y - min_y) };
}

Rect2I Rect2I::create_aligned(s32 origin_x, s32 origin_y, s32 w, s32 h, Alignment alignment)
{
    s32 x = 0;
    s32 y = 0;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        x = origin_x - (w / 2);
        break;
    case HAlign::Right:
        x = origin_x - w;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Meaningless here so default to left
    default:
        x = origin_x;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        y = origin_y - (h / 2);
        break;
    case VAlign::Bottom:
        y = origin_y - h;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Meaningless here so default to top
    default:
        y = origin_y;
        break;
    }

    return { x, y, w, h };
}

Rect2I Rect2I::create_aligned(V2I origin, V2I size, Alignment alignment)
{
    return create_aligned(origin.x, origin.y, size.x, size.y, alignment);
}

bool Rect2I::contains(s32 x, s32 y) const
{
    return min_x() <= x
        && x < max_x()
        && min_y() <= y
        && y < max_y();
}

bool Rect2I::contains(V2I pos) const
{
    return contains(pos.x, pos.y);
}

bool Rect2I::contains(Rect2I const& inner) const
{
    return contains(inner.position()) && contains(inner.position() + inner.size() - v2i(1, 1));
}

bool Rect2I::overlaps(Rect2I const& other) const
{
    return min_x() < other.max_x()
        && other.min_x() < max_x()
        && min_y() < other.max_y()
        && other.min_y() < max_y();
}

Rect2I Rect2I::expanded(s32 radius) const
{
    return expanded(radius, radius, radius, radius);
}

Rect2I Rect2I::expanded(s32 top, s32 right, s32 bottom, s32 left) const
{
    return {
        m_x - left,
        m_y - top,
        m_width + left + right,
        m_height + top + bottom
    };
}

Rect2I Rect2I::shrunk(s32 radius) const
{
    return shrunk(radius, radius, radius, radius);
}

Rect2I Rect2I::shrunk(s32 top, s32 right, s32 bottom, s32 left) const
{
    Rect2I result {
        m_x + left,
        m_y + top,
        m_width - (left + right),
        m_height - (top + bottom)
    };

    // NB: The shrink amount might be different from one side to the other, which means the new
    // centre is not the same as the old centre!
    // So, we create the new, possibly inside-out rectangle, and then set its dimensions
    // to 0 if they are negative, while maintaining the centre point.
    // FIXME: This means that shrinking past 0 will move the rectangle around. That seems not OK?

    V2 centre = result.centre();

    if (result.m_width < 0) {
        result.m_x = (s32)centre.x;
        result.m_width = 0;
    }

    if (result.m_height < 0) {
        result.m_y = (s32)centre.y;
        result.m_height = 0;
    }

    return result;
}

Rect2I Rect2I::shrunk(Padding const& padding) const
{
    return shrunk(padding.top, padding.right, padding.bottom, padding.left);
}

Rect2I Rect2I::intersected(Rect2I const& other) const
{
    Rect2I result = *this;

    // X
    s32 rightExtension = (result.m_x + result.m_width) - (other.m_x + other.m_width);
    if (rightExtension > 0) {
        result.m_width -= rightExtension;
    }
    if (result.m_x < other.m_x) {
        s32 leftExtension = other.m_x - result.m_x;
        result.m_x += leftExtension;
        result.m_width -= leftExtension;
    }

    // Y
    s32 bottomExtension = (result.m_y + result.m_height) - (other.m_y + other.m_height);
    if (bottomExtension > 0) {
        result.m_height -= bottomExtension;
    }
    if (result.m_y < other.m_y) {
        s32 topExtension = other.m_y - result.m_y;
        result.m_y += topExtension;
        result.m_height -= topExtension;
    }

    result.m_width = max(result.m_width, 0);
    result.m_height = max(result.m_height, 0);

    return result;
}

Rect2I Rect2I::intersected_relative(Rect2I const& inner) const
{
    Rect2I result = intersected(inner);
    result.set_position(result.position() - position());

    return result;
}

Rect2I Rect2I::union_with(Rect2I const& other) const
{
    s32 min_x = min(m_x, other.m_x);
    s32 min_y = min(m_y, other.m_y);
    s32 max_x = max(m_x + m_width - 1, other.m_x + other.m_width - 1);
    s32 max_y = max(m_y + m_height - 1, other.m_y + other.m_height - 1);

    return Rect2I::create_min_max(min_x, min_y, max_x, max_y);
}

V2 Rect2I::centre() const
{
    return v2(m_x + m_width / 2.0f, m_y + m_height / 2.0f);
}

s32 Rect2I::area() const
{
    return abs_s32(m_width * m_height);
}

bool Rect2I::has_positive_area() const
{
    return m_width > 0 && m_height > 0;
}

Rect2I Rect2I::create_centred_within(V2I inner_size) const
{
    return {
        m_x - ((inner_size.x - m_width) / 2),
        m_y - ((inner_size.y - m_height) / 2),
        inner_size.x,
        inner_size.y
    };
}

Rect2I Rect2I::create_aligned_within(V2I size, Alignment alignment, Padding padding) const
{
    V2I origin;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        origin.x = m_x + (m_width / 2);
        break;
    case HAlign::Right:
        origin.x = m_x + m_width - padding.right;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Meaningless here so default to left
    default:
        origin.x = m_x + padding.left;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        origin.y = m_y + (m_height / 2);
        break;
    case VAlign::Bottom:
        origin.y = m_y + m_height - padding.bottom;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Meaningless here so default to top
    default:
        origin.y = m_y + padding.top;
        break;
    }

    return Rect2I::create_aligned(origin, size, alignment);
}
