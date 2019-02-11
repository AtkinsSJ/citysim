#pragma once

inline s32 clamp(s32 value, s32 min, s32 max)
{
	ASSERT(min < max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}
inline f32 clamp(f32 value, f32 min, f32 max)
{
	ASSERT(min < max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

/**********************************************
	COORD
 **********************************************/

inline V2I v2i(s32 x, s32 y)
{
	return {x,y};
}
inline V2I v2i(V2 v2)
{
	return {(s32)v2.x, (s32)v2.y};
}

inline V2I operator+(V2I lhs, V2I rhs)
{
	V2I result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}
inline V2I operator+=(V2I &lhs, V2I rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V2I operator-(V2I lhs, V2I rhs)
{
	V2I result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}
inline V2I operator-=(V2I &lhs, V2I rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V2I operator*(V2I v, s32 s)
{
	V2I result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline V2I operator*=(V2I &v, s32 s)
{
	v = v * s;
	return v;
}
inline V2I operator/(V2I v, s32 s)
{
	V2I result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline V2I operator/=(V2I &v, s32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V2
 **********************************************/

inline V2 v2(V2I coord)
{
	return {(f32)coord.x, (f32)coord.y};
}
inline V2 v2(f32 x, f32 y)
{
	return {x,y};
}
inline V2 v2(int x, int y)
{
	return {(f32)x, (f32)y};
}

inline f32 v2Length(V2 v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y);
}

inline V2 operator+(V2 lhs, V2 rhs)
{
	V2 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}
inline V2 operator+=(V2 &lhs, V2 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V2 operator-(V2 lhs, V2 rhs)
{
	V2 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}
inline V2 operator-=(V2 &lhs, V2 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V2 operator*(V2 v, f32 s)
{
	V2 result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline V2 operator*=(V2 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V2 operator/(V2 v, f32 s)
{
	V2 result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline V2 operator/=(V2 &v, f32 s)
{
	v = v / s;
	return v;
}

inline V2 limit(V2 vector, f32 maxLength)
{
	f32 length = v2Length(vector);
	if (length > maxLength)
	{
		vector *= maxLength / length;
	}
	return vector;
}

/**********************************************
	V3
 **********************************************/

inline V3 v3(f32 x, f32 y, f32 z)
{
	V3 v = {};
	v.x = x;
	v.y = y;
	v.z = z;

	return v;
}

inline f32 v3Length(V3 v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline V3 operator+(V3 lhs, V3 rhs)
{
	V3 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	result.z = lhs.z + rhs.z;
	return result;
}
inline V3 operator+=(V3 &lhs, V3 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V3 operator-(V3 lhs, V3 rhs)
{
	V3 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	result.z = lhs.z - rhs.z;
	return result;
}
inline V3 operator-=(V3 &lhs, V3 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V3 operator*(V3 v, f32 s)
{
	V3 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}
inline V3 operator*=(V3 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V3 operator/(V3 v, f32 s)
{
	V3 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	return result;
}
inline V3 operator/=(V3 &v, f32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V4
 **********************************************/

inline V4 v4(f32 x, f32 y, f32 z, f32 w)
{
	V4 v = {};
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;

	return v;
}

inline V4 color255(u8 r, u8 g, u8 b, u8 a)
{
	V4 v = {};
	v.a = (f32)a / 255.0f;

	// NB: Prremultiplied alpha!
	v.r = v.a * ((f32)r / 255.0f);
	v.g = v.a * ((f32)g / 255.0f);
	v.b = v.a * ((f32)b / 255.0f);

	return v;
}
inline V4 makeWhite()
{
	return v4(1.0f,1.0f,1.0f,1.0f);
}

inline f32 v4Length(V4 v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline V4 operator+(V4 lhs, V4 rhs)
{
	V4 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	result.z = lhs.z + rhs.z;
	result.w = lhs.w + rhs.w;
	return result;
}
inline V4 operator+=(V4 &lhs, V4 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V4 operator-(V4 lhs, V4 rhs)
{
	V4 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	result.z = lhs.z - rhs.z;
	result.w = lhs.w - rhs.w;
	return result;
}
inline V4 operator-=(V4 &lhs, V4 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V4 operator*(V4 v, f32 s)
{
	V4 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;
	return result;
}
inline V4 operator*=(V4 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V4 operator/(V4 v, f32 s)
{
	V4 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	result.w = v.w / s;
	return result;
}
inline V4 operator/=(V4 &v, f32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	Rectangle (int)
 **********************************************/

inline Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h)
{
	Rect2I rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

// Rectangle that is guaranteed to not contain anything, because it's inside-out
inline Rect2I irectNegativeInfinity() {
	Rect2I rect = {};
	rect.x = s32Max;
	rect.y = s32Max;
	rect.w = s32Min;
	rect.h = s32Min;
	return rect;
}

inline Rect2I irectPosDim(V2I position, V2I dim)
{
	Rect2I rect = {};
	rect.pos = position;
	rect.dim = dim;
	return rect;
}

inline Rect2I irectCentreWH(V2I position, s32 w, s32 h)
{
	Rect2I rect = {};
	rect.x = position.x - w/2;
	rect.y = position.y - h/2;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline Rect2I irectCentreDim(V2I position, V2I dim)
{
	Rect2I rect = {};
	rect.x = position.x - dim.x/2;
	rect.y = position.y - dim.y/2;
	rect.dim = dim;
	return rect;
}

inline Rect2I irectCentreDim(V2 position, V2 dim)
{
	Rect2I rect = {};
	rect.x = (s32) (position.x - dim.x/2.0f);
	rect.y = (s32) (position.y - dim.y/2.0f);
	rect.w = (s32) dim.x;
	rect.h = (s32) dim.y;
	return rect;
}

inline Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax)
{
	return irectXYWH(xMin, yMin, (1+xMax-xMin), (1+yMax-yMin));
}

inline Rect2I irectCovering(V2I a, V2I b)
{
	Rect2I rect = {};
	if (a.x < b.x)
	{
		rect.x = a.x;
		rect.w = b.x - a.x + 1;
	}
	else
	{
		rect.x = b.x;
		rect.w = a.x - b.x + 1;
	}

	if (a.y < b.y)
	{
		rect.y = a.y;
		rect.h = b.y - a.y + 1;
	}
	else
	{
		rect.y = b.y;
		rect.h = a.y - b.y + 1;
	}
	return rect;
}

inline bool inRect2I(Rect2I rect, V2I coord)
{
	return coord.x >= rect.x
		&& coord.x < (rect.x + rect.w)
		&& coord.y >= rect.y
		&& coord.y < (rect.y + rect.h);
}

inline bool inRect2I(Rect2I rect, V2 pos)
{
	return pos.x >= rect.x
		&& pos.x < (rect.x + rect.w)
		&& pos.y >= rect.y
		&& pos.y < (rect.y + rect.h);
}

inline bool rectInRect2I(Rect2I outer, Rect2I inner)
{
	return inner.x >= outer.x
		&& (inner.x + inner.w) <= (outer.x + outer.w)
		&& inner.y >= outer.y
		&& (inner.y + inner.h) <= (outer.y + outer.h);
}

inline Rect2I expand(Rect2I rect, s32 addRadius)
{
	return irectXYWH(
		rect.x - addRadius,
		rect.y - addRadius,
		rect.w + (addRadius * 2),
		rect.h + (addRadius * 2)
	);
}

inline bool rectsOverlap(Rect2I a, Rect2I b)
{
	return (a.x < b.x + b.w)
		&& (b.x < a.x + a.w)
		&& (a.y < b.y + b.h)
		&& (b.y < a.y + a.h);
}

inline V2 centre(Rect2I rect)
{
	return v2(
		(f32)rect.x + (f32)rect.w / 2.0f,
		(f32)rect.y + (f32)rect.h / 2.0f
	);
}

inline V2I iCentre(Rect2I rect)
{
	return v2i(rect.x + rect.w/2, rect.y + rect.h/2);
}

/**********************************************
	Rect2
 **********************************************/

inline Rect2 rect2(V2 pos, f32 w, f32 h)
{
	Rect2 rect = {};
	rect.pos = pos;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline Rect2 rect2(Rect2I intRect2I)
{
	Rect2 rect = {};
	rect.x = (f32) intRect2I.x;
	rect.y = (f32) intRect2I.y;
	rect.w = (f32) intRect2I.w;
	rect.h = (f32) intRect2I.h;
	return rect;
}

inline Rect2 rectXYWH(f32 x, f32 y, f32 w, f32 h)
{
	Rect2 rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline Rect2 rectXYWHi(s32 x, s32 y, s32 w, s32 h)
{
	Rect2 rect = {};
	rect.x = (f32) x;
	rect.y = (f32) y;
	rect.w = (f32) w;
	rect.h = (f32) h;
	return rect;
}

inline Rect2 rectPosSize(V2 pos, V2 size)
{
	Rect2 rect = {};
	rect.pos  = pos;
	rect.size = size;
	return rect;
}

inline Rect2 rectCentreSize(V2 centre, V2 size)
{
	Rect2 rect = {};
	rect.x = centre.x - size.x/2.0f;
	rect.y = centre.y - size.y/2.0f;
	rect.w = size.x;
	rect.h = size.y;
	return rect;
}

inline Rect2 rectAligned(V2 origin, V2 size, u32 alignment)
{
	Rect2 rect = {};
	rect.size = size;

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE: {
			rect.x = origin.x - round_f32(size.x / 2.0f);
		} break;

		case ALIGN_RIGHT: {
			rect.x = origin.x - size.x;
		} break;

		default: { // Left is default
			rect.x = origin.x;
		} break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE: {
			rect.y = origin.y - round_f32(size.y / 2.0f);
		} break;

		case ALIGN_BOTTOM: {
			rect.y = origin.y - size.y;
		} break;

		default: { // Top is default
			rect.y = origin.y;
		} break;
	}

	return rect;
}

inline Rect2 expand(Rect2 rect, f32 addRadius)
{
	return rectXYWH(
		rect.x - addRadius,
		rect.y - addRadius,
		rect.w + (addRadius * 2.0f),
		rect.h + (addRadius * 2.0f)
	);
}

inline Rect2 offset(Rect2 rect, V2 offset)
{
	Rect2 result = rect;
	result.pos += offset;

	return result;
}

inline bool inRect(Rect2 rect, V2 pos)
{
	return pos.x >= rect.x
		&& pos.x < (rect.x + rect.w)
		&& pos.y >= rect.y
		&& pos.y < (rect.y + rect.h);
}

inline bool inRects(Rect2 *rects, s32 rectCount, V2 pos)
{
	bool result = false;
	for (int i=0; i < rectCount; i++)
	{
		if (inRect(rects[i], pos))
		{
			result = true;
			break;
		}
	}
	return result;
}

inline V2 centre(Rect2 rect)
{
	return v2(
		rect.x + rect.w / 2.0f,
		rect.y + rect.h / 2.0f
	);
}