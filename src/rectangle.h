#pragma once

struct Rect2
{
	union
	{
		V2 pos;
		struct
		{
			f32 x;
			f32 y;
		};
	};
	union
	{
		V2 size;
		struct
		{
			f32 w;
			f32 h;
		};
	};
};

Rect2 rectXYWH(f32 x, f32 y, f32 w, f32 h);
Rect2 rectXYWHi(s32 x, s32 y, s32 w, s32 h);
Rect2 rectPosSize(V2 pos, V2 size);
Rect2 rectCentreSize(V2 centre, V2 size);
Rect2 rectMinMax(f32 minX, f32 minY, f32 maxX, f32 maxY);
Rect2 rectAligned(V2 origin, V2 size, u32 alignment);
Rect2 rect2(Rect2I source);

bool contains(Rect2 rect, f32 x, f32 y);
bool contains(Rect2 rect, V2 pos);
bool contains(Rect2 outer, Rect2 inner);
bool overlaps(Rect2 a, Rect2 b);

Rect2 expand(Rect2 rect, f32 radius);
Rect2 expand(Rect2 rect, f32 top, f32 right, f32 bottom, f32 left);
Rect2 intersect(Rect2 a, Rect2 b);
Rect2 intersectRelative(Rect2 outer, Rect2 inner);

V2 centreOf(Rect2 rect);
f32 areaOf(Rect2 rect); // Always positive, even if the rect has negative dimensions
bool hasPositiveArea(Rect2 rect);

V2 alignWithinRectangle(Rect2 bounds, u32 alignment, f32 padding=0);

struct Rect2I
{
	union
	{
		V2I pos;
		struct
		{
			s32 x;
			s32 y;
		};
	};
	union
	{
		V2I size;
		struct
		{
			s32 w;
			s32 h;
		};
	};
};

Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h);
Rect2I irectInfinity(); // Contains approximately everything...
Rect2I irectNegativeInfinity(); // Contains nothing
Rect2I irectPosSize(V2I position, V2I size);
Rect2I irectCentreSize(s32 centreX, s32 centreY, s32 sizeX, s32 sizeY);
Rect2I irectCentreSize(V2I position, V2I size);
Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax);
Rect2I irectAligned(s32 originX, s32 originY, s32 w, s32 h, u32 alignment);
Rect2I irectAligned(V2I origin, V2I size, u32 alignment);

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
V2I alignWithinRectangle(Rect2I bounds, u32 alignment, Padding padding = {});
Rect2I alignWithinRectangle(Rect2I bounds, V2I size, u32 alignment, Padding padding = {});
