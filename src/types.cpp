#pragma once

inline s32 clamp(s32 value, s32 min, s32 max)
{
	ASSERT(min <= max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

inline f32 clamp(f32 value, f32 min, f32 max)
{
	ASSERT(min <= max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

inline s32 min(s32 a, s32 b)
{
	return (a < b) ? a : b;
}

inline f32 min(f32 a, f32 b)
{
	return (a < b) ? a : b;
}

inline s32 max(s32 a, s32 b)
{
	return (a > b) ? a : b;
}

inline f32 max(f32 a, f32 b)
{
	return (a > b) ? a : b;
}

inline u32 wrap(u32 value, u32 max)
{
	return (value + max) % max;
}

inline s32 truncate32(s64 in)
{
	ASSERT(in <= s32Max, "Value is too large to truncate to s32!");
	return (s32) in;
}

// Standard rounding functions return doubles, so here's some int ones.
inline s32 round_s32(f32 in)
{
	return (s32) round(in);
}

inline s32 floor_s32(f32 in)
{
	return (s32) floor(in);
}

inline s32 ceil_s32(f32 in)
{
	return (s32) ceil(in);
}

inline f32 round_f32(f32 in)
{
	return (f32) round(in);
}

inline f32 floor_f32(f32 in)
{
	return (f32) floor(in);
}

inline f32 ceil_f32(f32 in)
{
	return (f32) ceil(in);
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
	static const f32 inv255 = 1.0f / 255.0f;
	
	V4 v = {};
	v.a = (f32)a * inv255;

	// NB: Premultiplied alpha!
	v.r = v.a * ((f32)r * inv255);
	v.g = v.a * ((f32)g * inv255);
	v.b = v.a * ((f32)b * inv255);

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
	Rect2
 **********************************************/


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
	return rectXYWH((f32) x, (f32) y, (f32) w, (f32) h);
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
	rect.pos = centre - (size * 0.5f);
	rect.size = size;
	return rect;
}

inline Rect2 rect2(Rect2I source)
{
	return rectXYWHi(source.x, source.y, source.w, source.h);
}

inline Rect2 rectAligned(V2 origin, V2 size, u32 alignment)
{
	Rect2 rect = {};
	rect.size = size;

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  rect.x = origin.x - round_f32(size.x / 2.0f);  break;
		case ALIGN_RIGHT:     rect.x = origin.x - size.x;                    break;
		case ALIGN_LEFT:      // Left is default
		default:              rect.x = origin.x;                             break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  rect.y = origin.y - round_f32(size.y / 2.0f);  break;
		case ALIGN_BOTTOM:    rect.y = origin.y - size.y;                    break;
		case ALIGN_TOP:       // Top is default
		default:              rect.y = origin.y;                             break;
	}

	return rect;
}

inline bool contains(Rect2 rect, f32 x, f32 y)
{
	return x >= rect.x
		&& x < (rect.x + rect.w)
		&& y >= rect.y
		&& y < (rect.y + rect.h);
}

inline bool contains(Rect2 rect, V2 pos)
{
	return contains(rect, pos.x, pos.y);
}

inline bool contains(Rect2 outer, Rect2 inner)
{
	return contains(outer, inner.pos) && contains(outer, inner.pos + inner.size);
}

inline Rect2 expand(Rect2 rect, f32 radius)
{
	return rectXYWH(
		rect.x - radius,
		rect.y - radius,
		rect.w + (radius * 2.0f),
		rect.h + (radius * 2.0f)
	);
}

inline Rect2 expand(Rect2 rect, f32 top, f32 right, f32 bottom, f32 left)
{
	return rectXYWH(
		rect.x - left,
		rect.y - top,
		rect.w + left + right,
		rect.h + top + bottom
	);
}

inline Rect2 intersect(Rect2 inner, Rect2 outer)
{
	Rect2 result = inner;

	// X
	f32 rightExtension = (result.x + result.w) - (outer.x + outer.w);
	if (rightExtension > 0)
	{
		result.w -= rightExtension;
	}
	if (result.x < outer.x)
	{
		f32 leftExtension = outer.x - result.x;
		result.x += leftExtension;
		result.w -= leftExtension;
	}

	// Y
	f32 bottomExtension = (result.y + result.h) - (outer.y + outer.h);
	if (bottomExtension > 0)
	{
		result.h -= bottomExtension;
	}
	if (result.y < outer.y)
	{
		f32 topExtension = outer.y - result.y;
		result.y += topExtension;
		result.h -= topExtension;
	}

	return result;
}

// Takes the intersection of inner and outer, and then converts it to being relative to outer.
// (Originally used to take a world-space rectangle and put it into a cropped, sector-space one.)
inline Rect2 intersectRelative(Rect2 outer, Rect2 inner)
{
	Rect2 result = intersect(inner, outer);
	result.pos -= outer.pos;

	return result;
}

inline V2 centreOf(Rect2 rect)
{
	return v2(
		rect.x + rect.w / 2.0f,
		rect.y + rect.h / 2.0f
	);
}

inline f32 areaOf(Rect2 rect)
{
	return rect.w * rect.h;
}

inline V2 originWithinRectangle(Rect2 bounds, u32 alignment, f32 padding)
{
	V2 result = v2(0,0);

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  result.x = bounds.x + bounds.w / 2.0f;     break;
		case ALIGN_RIGHT:     result.x = bounds.x + bounds.w - padding;  break;
		case ALIGN_LEFT:      // Left is default
		default:              result.x = bounds.x + padding;             break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  result.y = bounds.y + bounds.h / 2.0f;     break;
		case ALIGN_BOTTOM:    result.y = bounds.y + bounds.h - padding;  break;
		case ALIGN_TOP:       // Top is default
		default:              result.y = bounds.y + padding;             break;
	}

	return result;
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

inline Rect2I irectPosSize(V2I position, V2I size)
{
	Rect2I rect = {};
	rect.pos = position;
	rect.size = size;
	return rect;
}

inline Rect2I irectCentreSize(V2I position, V2I size)
{
	Rect2I rect = {};
	rect.pos = position - (size / 2);
	rect.size = size;
	return rect;
}

inline Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax)
{
	return irectXYWH(xMin, yMin, (1+xMax-xMin), (1+yMax-yMin));
}

inline bool contains(Rect2I rect, s32 x, s32 y)
{
	return (x >= rect.x)
		&& (x < rect.x + rect.w)
		&& (y >= rect.y)
		&& (y < rect.y + rect.h);
}

inline bool contains(Rect2I rect, V2I pos)
{
	return contains(rect, pos.x, pos.y);
}

inline bool contains(Rect2I outer, Rect2I inner)
{
	return inner.x >= outer.x
		&& (inner.x + inner.w) <= (outer.x + outer.w)
		&& inner.y >= outer.y
		&& (inner.y + inner.h) <= (outer.y + outer.h);
}

inline bool overlaps(Rect2I a, Rect2I b)
{
	return (a.x < b.x + b.w)
		&& (b.x < a.x + a.w)
		&& (a.y < b.y + b.h)
		&& (b.y < a.y + a.h);
}


inline Rect2I expand(Rect2I rect, s32 radius)
{
	return irectXYWH(
		rect.x - radius,
		rect.y - radius,
		rect.w + (radius * 2),
		rect.h + (radius * 2)
	);
}

inline Rect2I expand(Rect2I rect, s32 top, s32 right, s32 bottom, s32 left)
{
	return irectXYWH(
		rect.x - left,
		rect.y - top,
		rect.w + left + right,
		rect.h + top + bottom
	);
}

inline Rect2I intersect(Rect2I inner, Rect2I outer)
{
	Rect2I result = inner;

	// X
	s32 rightExtension = (result.x + result.w) - (outer.x + outer.w);
	if (rightExtension > 0)
	{
		result.w -= rightExtension;
	}
	if (result.x < outer.x)
	{
		s32 leftExtension = outer.x - result.x;
		result.x += leftExtension;
		result.w -= leftExtension;
	}

	// Y
	s32 bottomExtension = (result.y + result.h) - (outer.y + outer.h);
	if (bottomExtension > 0)
	{
		result.h -= bottomExtension;
	}
	if (result.y < outer.y)
	{
		s32 topExtension = outer.y - result.y;
		result.y += topExtension;
		result.h -= topExtension;
	}

	return result;
}

// Takes the intersection of inner and outer, and then converts it to being relative to outer.
// (Originally used to take a world-space rectangle and put it into a cropped, sector-space one.)
inline Rect2I intersectRelative(Rect2I outer, Rect2I inner)
{
	Rect2I result = intersect(inner, outer);
	result.pos -= outer.pos;

	return result;
}

inline V2I centreOf(Rect2I rect)
{
	return v2i(rect.x + rect.w/2, rect.y + rect.h/2);
}

inline s32 areaOf(Rect2I rect)
{
	return rect.w * rect.h;
}
