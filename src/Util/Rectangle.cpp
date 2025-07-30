/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Rectangle.h"
#include <Util/Maths.h>

/**********************************************
    Rect2
 **********************************************/

Rect2 rectXYWH(float x, float y, float w, float h)
{
    Rect2 rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
}

Rect2 rectXYWHi(s32 x, s32 y, s32 w, s32 h)
{
    return rectXYWH((float)x, (float)y, (float)w, (float)h);
}

Rect2 rectPosSize(V2 pos, V2 size)
{
    Rect2 rect;
    rect.pos = pos;
    rect.size = size;
    return rect;
}

Rect2 rectCentreSize(V2 centre, V2 size)
{
    return rectPosSize(centre - (size * 0.5f), size);
}

Rect2 rect2(Rect2I source)
{
    return rectXYWHi(source.x, source.y, source.w, source.h);
}

Rect2 rectMinMax(float minX, float minY, float maxX, float maxY)
{
    Rect2 rect;

    rect.x = minX;
    rect.y = minY;
    rect.w = maxX - minX;
    rect.h = maxY - minY;

    return rect;
}

Rect2 rectAligned(V2 origin, V2 size, Alignment alignment)
{
    Rect2 rect = {};
    rect.size = size;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        rect.x = origin.x - round_float(size.x / 2.0f);
        break;
    case HAlign::Right:
        rect.x = origin.x - size.x;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Same as left
    default:
        rect.x = origin.x;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        rect.y = origin.y - round_float(size.y / 2.0f);
        break;
    case VAlign::Bottom:
        rect.y = origin.y - size.y;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Same as top
    default:
        rect.y = origin.y;
        break;
    }

    return rect;
}

bool contains(Rect2 rect, float x, float y)
{
    return x >= rect.x
        && x < (rect.x + rect.w)
        && y >= rect.y
        && y < (rect.y + rect.h);
}

bool contains(Rect2 rect, V2 pos)
{
    return contains(rect, pos.x, pos.y);
}

bool contains(Rect2 outer, Rect2 inner)
{
    return contains(outer, inner.pos) && contains(outer, inner.pos + inner.size);
}

bool overlaps(Rect2 a, Rect2 b)
{
    return (a.x < b.x + b.w)
        && (b.x < a.x + a.w)
        && (a.y < b.y + b.h)
        && (b.y < a.y + a.h);
}

Rect2 expand(Rect2 rect, float radius)
{
    return rectXYWH(
        rect.x - radius,
        rect.y - radius,
        rect.w + (radius * 2.0f),
        rect.h + (radius * 2.0f));
}

Rect2 expand(Rect2 rect, float top, float right, float bottom, float left)
{
    return rectXYWH(
        rect.x - left,
        rect.y - top,
        rect.w + left + right,
        rect.h + top + bottom);
}

Rect2 intersect(Rect2 inner, Rect2 outer)
{
    Rect2 result = inner;

    // X
    float rightExtension = (result.x + result.w) - (outer.x + outer.w);
    if (rightExtension > 0) {
        result.w -= rightExtension;
    }
    if (result.x < outer.x) {
        float leftExtension = outer.x - result.x;
        result.x += leftExtension;
        result.w -= leftExtension;
    }

    // Y
    float bottomExtension = (result.y + result.h) - (outer.y + outer.h);
    if (bottomExtension > 0) {
        result.h -= bottomExtension;
    }
    if (result.y < outer.y) {
        float topExtension = outer.y - result.y;
        result.y += topExtension;
        result.h -= topExtension;
    }

    result.w = max(result.w, 0.0f);
    result.h = max(result.h, 0.0f);

    return result;
}

// Takes the intersection of inner and outer, and then converts it to being relative to outer.
// (Originally used to take a world-space rectangle and put it into a cropped, sector-space one.)
Rect2 intersectRelative(Rect2 outer, Rect2 inner)
{
    Rect2 result = intersect(inner, outer);
    result.pos -= outer.pos;

    return result;
}

V2 centreOf(Rect2 rect)
{
    return v2(
        rect.x + rect.w / 2.0f,
        rect.y + rect.h / 2.0f);
}

V2I centreOfI(Rect2I rect)
{
    return v2i(
        round_s32(rect.x + rect.w / 2.0f),
        round_s32(rect.y + rect.h / 2.0f));
}

float areaOf(Rect2 rect)
{
    return abs_float(rect.w * rect.h);
}

bool hasPositiveArea(Rect2 rect)
{
    return (rect.w > 0 && rect.h > 0);
}

/**********************************************
        Rect2I
 **********************************************/

Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h)
{
    Rect2I rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    return rect;
}

// Rectangle that SHOULD contain anything. It is the maximum size, and centred
// on 0,0 In practice, the distance between the minimum and maximum integers
// is double what we can store, so the min and max possible positions are
// outside this rectangle, by a long way (0.5 * s32Max). But I cannot conceive
// that we will ever need to deal with positions that are anywhere near that,
// so it should be fine.
// - Sam, 08/02/2021
Rect2I irectInfinity()
{
    return irectXYWH(s32Min / 2, s32Min / 2, s32Max, s32Max);
}

// Rectangle that is guaranteed to not contain anything, because it's inside-out
Rect2I irectNegativeInfinity()
{
    return irectXYWH(s32Max, s32Max, s32Min, s32Min);
}

Rect2I irectPosSize(V2I position, V2I size)
{
    Rect2I rect;
    rect.pos = position;
    rect.size = size;
    return rect;
}

Rect2I irectCentreSize(s32 centreX, s32 centreY, s32 sizeX, s32 sizeY)
{
    return irectXYWH(centreX - (sizeX / 2), centreY - (sizeY / 2), sizeX, sizeY);
}

Rect2I irectCentreSize(V2I position, V2I size)
{
    return irectPosSize(position - (size / 2), size);
}

Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax)
{
    return irectXYWH(xMin, yMin, (1 + xMax - xMin), (1 + yMax - yMin));
}

Rect2I irectAligned(s32 originX, s32 originY, s32 w, s32 h, Alignment alignment)
{
    Rect2I rect = {};
    rect.w = w;
    rect.h = h;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        rect.x = originX - (w / 2);
        break;
    case HAlign::Right:
        rect.x = originX - w;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Meaningless here so default to left
    default:
        rect.x = originX;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        rect.y = originY - (h / 2);
        break;
    case VAlign::Bottom:
        rect.y = originY - h;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Meaningless here so default to top
    default:
        rect.y = originY;
        break;
    }

    return rect;
}

Rect2I irectAligned(V2I origin, V2I size, Alignment alignment)
{
    return irectAligned(origin.x, origin.y, size.x, size.y, alignment);
}

bool contains(Rect2I rect, s32 x, s32 y)
{
    return (x >= rect.x)
        && (x < rect.x + rect.w)
        && (y >= rect.y)
        && (y < rect.y + rect.h);
}

bool contains(Rect2I rect, V2I pos)
{
    return contains(rect, pos.x, pos.y);
}

bool contains(Rect2I rect, V2 pos)
{
    return (pos.x >= rect.x)
        && (pos.x < rect.x + rect.w)
        && (pos.y >= rect.y)
        && (pos.y < rect.y + rect.h);
}

bool contains(Rect2I outer, Rect2I inner)
{
    return inner.x >= outer.x
        && (inner.x + inner.w) <= (outer.x + outer.w)
        && inner.y >= outer.y
        && (inner.y + inner.h) <= (outer.y + outer.h);
}

bool overlaps(Rect2I a, Rect2I b)
{
    return (a.x < b.x + b.w)
        && (b.x < a.x + a.w)
        && (a.y < b.y + b.h)
        && (b.y < a.y + a.h);
}

Rect2I expand(Rect2I rect, s32 radius)
{
    return irectXYWH(
        rect.x - radius,
        rect.y - radius,
        rect.w + (radius * 2),
        rect.h + (radius * 2));
}

Rect2I expand(Rect2I rect, s32 top, s32 right, s32 bottom, s32 left)
{
    return irectXYWH(
        rect.x - left,
        rect.y - top,
        rect.w + left + right,
        rect.h + top + bottom);
}

Rect2I shrink(Rect2I rect, s32 radius)
{
    s32 doubleRadius = radius * 2;
    if ((rect.w >= doubleRadius) && (rect.h >= doubleRadius)) {
        return irectXYWH(
            rect.x + radius,
            rect.y + radius,
            rect.w - doubleRadius,
            rect.h - doubleRadius);
    } else {
        V2 centre = centreOf(rect);
        return irectCentreSize((s32)centre.x, (s32)centre.y,
            max(rect.w - doubleRadius, 0), max(rect.h - doubleRadius, 0));
    }
}

Rect2I shrink(Rect2I rect, Padding padding)
{
    Rect2I result = irectXYWH(
        rect.x + padding.left,
        rect.y + padding.top,
        rect.w - (padding.left + padding.right),
        rect.h - (padding.top + padding.bottom));

    // NB: We do this calculation differently from the simpler shrink(rect, radius), because
    // the shrink amount might be different from one side to the other, which means the new
    // centre is not the same as the old centre!
    // So, we create the new, possibly inside-out rectangle, and then set its dimensions
    // to 0 if they are negative, while maintaining the centre point.

    V2 centre = centreOf(result);

    if (result.w < 0) {
        result.x = (s32)centre.x;
        result.w = 0;
    }

    if (result.h < 0) {
        result.y = (s32)centre.y;
        result.h = 0;
    }

    return result;
}

Rect2I intersect(Rect2I inner, Rect2I outer)
{
    Rect2I result = inner;

    // X
    s32 rightExtension = (result.x + result.w) - (outer.x + outer.w);
    if (rightExtension > 0) {
        result.w -= rightExtension;
    }
    if (result.x < outer.x) {
        s32 leftExtension = outer.x - result.x;
        result.x += leftExtension;
        result.w -= leftExtension;
    }

    // Y
    s32 bottomExtension = (result.y + result.h) - (outer.y + outer.h);
    if (bottomExtension > 0) {
        result.h -= bottomExtension;
    }
    if (result.y < outer.y) {
        s32 topExtension = outer.y - result.y;
        result.y += topExtension;
        result.h -= topExtension;
    }

    result.w = max(result.w, 0);
    result.h = max(result.h, 0);

    return result;
}

Rect2I intersectRelative(Rect2I outer, Rect2I inner)
{
    Rect2I result = intersect(inner, outer);
    result.pos -= outer.pos;

    return result;
}

Rect2I unionOf(Rect2I a, Rect2I b)
{
    s32 minX = min(a.x, b.x);
    s32 minY = min(a.y, b.y);
    s32 maxX = max(a.x + a.w - 1, b.x + b.w - 1);
    s32 maxY = max(a.y + a.h - 1, b.y + b.h - 1);

    return irectMinMax(minX, minY, maxX, maxY);
}

V2 centreOf(Rect2I rect)
{
    return v2(rect.x + rect.w / 2.0f, rect.y + rect.h / 2.0f);
}

s32 areaOf(Rect2I rect)
{
    return abs_s32(rect.w * rect.h);
}

bool hasPositiveArea(Rect2I rect)
{
    return (rect.w > 0 && rect.h > 0);
}

Rect2I centreWithin(Rect2I outer, V2I innerSize)
{
    Rect2I result = irectXYWH(
        outer.x - ((innerSize.x - outer.w) / 2),
        outer.y - ((innerSize.y - outer.h) / 2),
        innerSize.x,
        innerSize.y);

    return result;
}

Rect2I alignWithinRectangle(Rect2I bounds, V2I size, Alignment alignment, Padding padding)
{
    V2I origin;

    switch (alignment.horizontal) {
    case HAlign::Centre:
        origin.x = bounds.x + (bounds.w / 2);
        break;
    case HAlign::Right:
        origin.x = bounds.x + bounds.w - padding.right;
        break;
    case HAlign::Left: // Left is default
    case HAlign::Fill: // Meaningless here so default to left
    default:
        origin.x = bounds.x + padding.left;
        break;
    }

    switch (alignment.vertical) {
    case VAlign::Centre:
        origin.y = bounds.y + (bounds.h / 2);
        break;
    case VAlign::Bottom:
        origin.y = bounds.y + bounds.h - padding.bottom;
        break;
    case VAlign::Top:  // Top is default
    case VAlign::Fill: // Meaningless here so default to top
    default:
        origin.y = bounds.y + padding.top;
        break;
    }

    return irectAligned(origin, size, alignment);
}
