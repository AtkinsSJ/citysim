#pragma once

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

inline s32 divideCeil(s32 numerator, s32 denominator)
{
	return (numerator + denominator - 1) / denominator;
}

inline s32 truncate32(s64 in)
{
	ASSERT(in <= s32Max, "Value is too large to truncate to s32!");
	return (s32) in;
}

inline bool equals(f32 a, f32 b, f32 epsilon)
{
	return (fabs(a-b) < epsilon);
}

template<typename T>
inline T clamp(T value, T min, T max)
{
	ASSERT(min <= max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

template<typename T>
inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
inline T wrap(T value, T max)
{
	return (value + max) % max;
}

template<typename T>
inline T lerp(T a, T b, f32 position)
{
	return a + (b-a)*position;
}

template<typename T>
inline T approach(T currentValue, T targetValue, T distance)
{
	T result = currentValue;

	if (targetValue < currentValue)
	{
		result = max(currentValue - distance, targetValue);
	}
	else
	{
		result = min(currentValue + distance, targetValue);
	}

	return result;
}

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
inline s32 manhattanDistance(Rect2I rect, V2I point)
{
	s32 result = 0;

	if (point.x < rect.x)
	{
		result += rect.x - point.x;
	}
	else if (point.x >= (rect.x + rect.w))
	{
		result += 1 + point.x - (rect.x + rect.w);
	}

	if (point.y < rect.y)
	{
		result += rect.y - point.y;
	}
	else if (point.y >= (rect.y + rect.h))
	{
		result += 1 + point.y - (rect.y + rect.h);
	}

	return result;
}

inline s32 manhattanDistance(Rect2I a, Rect2I b)
{
	s32 result = 0;

	if (a.x + a.w <= b.x)
	{
		result += 1 + b.x - (a.x + a.w);
	}
	else if (b.x + b.w <= a.x)
	{
		result += 1 + a.x - (b.x + b.w);
	}

	if (a.y + a.h <= b.y)
	{
		result += 1 + b.y - (a.y + a.h);
	}
	else if (b.y + b.h <= a.y)
	{
		result += 1 + a.y - (b.y + b.h);
	}
	
	return result;
}

/**********************************************
	V2
 **********************************************/

inline V2 v2(f32 x, f32 y)
{
	V2 result = {};
	result.x = x;
	result.y = y;
	return result;
}

inline V2 v2(s32 x, s32 y)
{
	return v2((f32)x, (f32)y);
}

inline V2 v2(V2I source)
{
	return v2(source.x, source.y);
}

inline f32 lengthOf(V2 v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y);
}

inline V2 limit(V2 vector, f32 maxLength)
{
	f32 length = lengthOf(vector);
	if (length > maxLength)
	{
		vector *= maxLength / length;
	}
	return vector;
}

inline V2 lerp(V2 a, V2 b, f32 position)
{
	return a + (b-a)*position;
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

/**********************************************
	V2I
 **********************************************/

inline V2I v2i(s32 x, s32 y)
{
	V2I result = {};
	result.x = x;
	result.y = y;
	return result;
}

inline V2I v2i(V2 source)
{
	return v2i((s32)source.x, (s32)source.y);
}

f32 lengthOf(V2I v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y);
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

inline V3 v3(s32 x, s32 y, s32 z)
{
	return v3((f32) x, (f32) y, (f32) z);
}

inline f32 lengthOf(V3 v)
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

inline V4 v4(s32 x, s32 y, s32 z, s32 w)
{
	return v4((f32) x, (f32) y, (f32) z, (f32) w);
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

inline f32 lengthOf(V4 v)
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
	Rect2 rect;
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
	Rect2 rect;
	rect.pos  = pos;
	rect.size = size;
	return rect;
}

inline Rect2 rectCentreSize(V2 centre, V2 size)
{
	return rectPosSize(centre - (size * 0.5f), size);
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

inline V2 alignWithinRectangle(Rect2 bounds, u32 alignment, f32 padding)
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

inline Rect2I centreWithin(Rect2I outer, Rect2I inner)
{
	Rect2I result = inner;

	result.x = outer.x - ((result.w - outer.w) / 2);
	result.y = outer.y - ((result.h - outer.h) / 2);

	return result;
}

//
// Matrix4
//
inline Matrix4 identityMatrix4()
{
	Matrix4 m = {};
	m.v[0][0] = 1;
	m.v[1][1] = 1;
	m.v[2][2] = 1;
	m.v[3][3] = 1;

	return m;
}

Matrix4 orthographicMatrix4(f32 left, f32 right, f32 top, f32 bottom, f32 nearClip, f32 farClip)
{
	Matrix4 m = {};
	m.v[0][0] = 2.0f / (right-left);
	m.v[1][1] = 2.0f / (top-bottom);
	m.v[2][2] = -2.0f / (farClip-nearClip);

	m.v[3][0] = -(right+left) / (right-left);
	m.v[3][1] = -(top+bottom) / (top-bottom);
	m.v[3][2] = -(farClip+nearClip) / (farClip-nearClip);
	m.v[3][3] = 1.0f;

	return m;
}

Matrix4 inverse(Matrix4 *source)
{
	DEBUG_FUNCTION();
  
	// Shamelessly copied from http://stackoverflow.com/a/1148405/1178345
	// This is complicated! *_*
	Matrix4 result = {};

	result.flat[0] = source->flat[5]  * source->flat[10] * source->flat[15] - 
	                  source->flat[5]  * source->flat[11] * source->flat[14] - 
	                  source->flat[9]  * source->flat[6]  * source->flat[15] + 
	                  source->flat[9]  * source->flat[7]  * source->flat[14] +
	                  source->flat[13] * source->flat[6]  * source->flat[11] - 
	                  source->flat[13] * source->flat[7]  * source->flat[10];

	result.flat[4] = -source->flat[4]  * source->flat[10] * source->flat[15] + 
	                  source->flat[4]  * source->flat[11] * source->flat[14] + 
	                  source->flat[8]  * source->flat[6]  * source->flat[15] - 
	                  source->flat[8]  * source->flat[7]  * source->flat[14] - 
	                  source->flat[12] * source->flat[6]  * source->flat[11] + 
	                  source->flat[12] * source->flat[7]  * source->flat[10];

	result.flat[8] = source->flat[4]  * source->flat[9] * source->flat[15] - 
	                  source->flat[4]  * source->flat[11] * source->flat[13] - 
	                  source->flat[8]  * source->flat[5] * source->flat[15] + 
	                  source->flat[8]  * source->flat[7] * source->flat[13] + 
	                  source->flat[12] * source->flat[5] * source->flat[11] - 
	                  source->flat[12] * source->flat[7] * source->flat[9];

	result.flat[12] = -source->flat[4]  * source->flat[9] * source->flat[14] + 
	                  source->flat[4]  * source->flat[10] * source->flat[13] +
	                  source->flat[8]  * source->flat[5] * source->flat[14] - 
	                  source->flat[8]  * source->flat[6] * source->flat[13] - 
	                  source->flat[12] * source->flat[5] * source->flat[10] + 
	                  source->flat[12] * source->flat[6] * source->flat[9];

	result.flat[1] = -source->flat[1]  * source->flat[10] * source->flat[15] + 
	                  source->flat[1]  * source->flat[11] * source->flat[14] + 
	                  source->flat[9]  * source->flat[2] * source->flat[15] - 
	                  source->flat[9]  * source->flat[3] * source->flat[14] - 
	                  source->flat[13] * source->flat[2] * source->flat[11] + 
	                  source->flat[13] * source->flat[3] * source->flat[10];

	result.flat[5] = source->flat[0]  * source->flat[10] * source->flat[15] - 
	                  source->flat[0]  * source->flat[11] * source->flat[14] - 
	                  source->flat[8]  * source->flat[2] * source->flat[15] + 
	                  source->flat[8]  * source->flat[3] * source->flat[14] + 
	                  source->flat[12] * source->flat[2] * source->flat[11] - 
	                  source->flat[12] * source->flat[3] * source->flat[10];

	result.flat[9] = -source->flat[0]  * source->flat[9] * source->flat[15] + 
	                  source->flat[0]  * source->flat[11] * source->flat[13] + 
	                  source->flat[8]  * source->flat[1] * source->flat[15] - 
	                  source->flat[8]  * source->flat[3] * source->flat[13] - 
	                  source->flat[12] * source->flat[1] * source->flat[11] + 
	                  source->flat[12] * source->flat[3] * source->flat[9];

	result.flat[13] = source->flat[0]  * source->flat[9] * source->flat[14] - 
	                  source->flat[0]  * source->flat[10] * source->flat[13] - 
	                  source->flat[8]  * source->flat[1] * source->flat[14] + 
	                  source->flat[8]  * source->flat[2] * source->flat[13] + 
	                  source->flat[12] * source->flat[1] * source->flat[10] - 
	                  source->flat[12] * source->flat[2] * source->flat[9];

	result.flat[2] = source->flat[1]  * source->flat[6] * source->flat[15] - 
	                  source->flat[1]  * source->flat[7] * source->flat[14] - 
	                  source->flat[5]  * source->flat[2] * source->flat[15] + 
	                  source->flat[5]  * source->flat[3] * source->flat[14] + 
	                  source->flat[13] * source->flat[2] * source->flat[7] - 
	                  source->flat[13] * source->flat[3] * source->flat[6];

	result.flat[6] = -source->flat[0]  * source->flat[6] * source->flat[15] + 
	                  source->flat[0]  * source->flat[7] * source->flat[14] + 
	                  source->flat[4]  * source->flat[2] * source->flat[15] - 
	                  source->flat[4]  * source->flat[3] * source->flat[14] - 
	                  source->flat[12] * source->flat[2] * source->flat[7] + 
	                  source->flat[12] * source->flat[3] * source->flat[6];

	result.flat[10] = source->flat[0]  * source->flat[5] * source->flat[15] - 
	                  source->flat[0]  * source->flat[7] * source->flat[13] - 
	                  source->flat[4]  * source->flat[1] * source->flat[15] + 
	                  source->flat[4]  * source->flat[3] * source->flat[13] + 
	                  source->flat[12] * source->flat[1] * source->flat[7] - 
	                  source->flat[12] * source->flat[3] * source->flat[5];

	result.flat[14] = -source->flat[0]  * source->flat[5] * source->flat[14] + 
	                  source->flat[0]  * source->flat[6] * source->flat[13] + 
	                  source->flat[4]  * source->flat[1] * source->flat[14] - 
	                  source->flat[4]  * source->flat[2] * source->flat[13] - 
	                  source->flat[12] * source->flat[1] * source->flat[6] + 
	                  source->flat[12] * source->flat[2] * source->flat[5];

	result.flat[3] = -source->flat[1] * source->flat[6] * source->flat[11] + 
	                  source->flat[1] * source->flat[7] * source->flat[10] + 
	                  source->flat[5] * source->flat[2] * source->flat[11] - 
	                  source->flat[5] * source->flat[3] * source->flat[10] - 
	                  source->flat[9] * source->flat[2] * source->flat[7] + 
	                  source->flat[9] * source->flat[3] * source->flat[6];

	result.flat[7] = source->flat[0] * source->flat[6] * source->flat[11] - 
	                  source->flat[0] * source->flat[7] * source->flat[10] - 
	                  source->flat[4] * source->flat[2] * source->flat[11] + 
	                  source->flat[4] * source->flat[3] * source->flat[10] + 
	                  source->flat[8] * source->flat[2] * source->flat[7] - 
	                  source->flat[8] * source->flat[3] * source->flat[6];

	result.flat[11] = -source->flat[0] * source->flat[5] * source->flat[11] + 
	                  source->flat[0] * source->flat[7] * source->flat[9] + 
	                  source->flat[4] * source->flat[1] * source->flat[11] - 
	                  source->flat[4] * source->flat[3] * source->flat[9] - 
	                  source->flat[8] * source->flat[1] * source->flat[7] + 
	                  source->flat[8] * source->flat[3] * source->flat[5];

	result.flat[15] = source->flat[0] * source->flat[5] * source->flat[10] - 
	                  source->flat[0] * source->flat[6] * source->flat[9] - 
	                  source->flat[4] * source->flat[1] * source->flat[10] + 
	                  source->flat[4] * source->flat[2] * source->flat[9] + 
	                  source->flat[8] * source->flat[1] * source->flat[6] - 
	                  source->flat[8] * source->flat[2] * source->flat[5];

	f32 det = source->flat[0] * result.flat[0] + source->flat[1] * result.flat[4] + source->flat[2] * result.flat[8] + source->flat[3] * result.flat[12];
	if (det != 0) {
		det = 1.0f / det;
		for (int i=0; i<16; i++) {
			result.flat[i] *= det;
		}
	}

	return result;
}

void translate(Matrix4 *matrix, V3 translation)
{
	matrix->v[3][0] += translation.x;
	matrix->v[3][1] += translation.y;
	matrix->v[3][2] += translation.z;
}

void scale(Matrix4 *matrix, V3 scale)
{
	matrix->v[0][0] *= scale.x;
	matrix->v[1][1] *= scale.y;
	matrix->v[2][2] *= scale.z;
}

void rotateZ(Matrix4 *matrix, f32 radians)
{
	Matrix4 rotation = identityMatrix4();
	f32 c = (f32) cos(radians);
	f32 s = (f32) sin(radians);
	rotation.v[0][0] = c;
	rotation.v[0][1] = s;
	rotation.v[1][0] = -s;
	rotation.v[1][1] = c;

	(*matrix) *= rotation;
}

inline Matrix4 operator+(Matrix4 a, Matrix4 b) {
	Matrix4 result = {};
	
	for (int i=0; i<16; i++) {
		result.flat[i] = a.flat[i] + b.flat[i];
	}

	return result;
}
inline Matrix4 operator+=(Matrix4 &a, Matrix4 b) {
	a = a + b;
	return a;
}

// Scalar multiplication
inline Matrix4 operator*(Matrix4 m, f32 f) {
	Matrix4 result = {};

	for (int i=0; i<16; i++) {
		result.flat[i] = m.flat[i] * f;
	}

	return result;
}

// Matrix multiplication
inline Matrix4 operator*(Matrix4 a, Matrix4 b) {
	Matrix4 result = {};

	for (int col=0; col<4; col++) {
		for (int row=0; row<4; row++) {

			result.v[col][row] = (a.v[0][row] * b.v[col][0])
							   + (a.v[1][row] * b.v[col][1])
							   + (a.v[2][row] * b.v[col][2])
							   + (a.v[3][row] * b.v[col][3]);
		}
	}

	return result;
}

inline Matrix4 operator*=(Matrix4 &a, Matrix4 b) {
	a = a * b;
	return a;
}

inline V4 operator*(Matrix4 m, V4 v)
{
	V4 result = {};

	result.x = v.x * m.v[0][0]
			 + v.y * m.v[1][0]
			 + v.z * m.v[2][0]
			 + v.w * m.v[3][0];

	result.y = v.x * m.v[0][1]
			 + v.y * m.v[1][1]
			 + v.z * m.v[2][1]
			 + v.w * m.v[3][1];

	result.z = v.x * m.v[0][2]
			 + v.y * m.v[1][2]
			 + v.z * m.v[2][2]
			 + v.w * m.v[3][2];

	result.w = v.x * m.v[0][3]
			 + v.y * m.v[1][3]
			 + v.z * m.v[2][3]
			 + v.w * m.v[3][3];

	return result;
}
