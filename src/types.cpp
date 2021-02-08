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

inline s32 abs_s32(s32 in)
{
	return (in < 0) ? -in : in;
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

inline f32 sqrt_f32(f32 in)
{
	return (f32) sqrt(in);
}

inline f32 abs_f32(f32 in)
{
	return (in < 0.0f) ? -in : in;
}

inline f32 fraction_f32(f32 in)
{
	return (f32) fmod(in, 1.0f);
}

inline f32 sin32(f32 radians)
{
	return (f32) sin(radians);
}

inline f32 cos32(f32 radians)
{
	return (f32) cos(radians);
}

inline f32 tan32(f32 radians)
{
	return (f32) tan(radians);
}


inline s32 divideCeil(s32 numerator, s32 denominator)
{
	return (numerator + denominator - 1) / denominator;
}

inline s32 truncate32(s64 in)
{
	ASSERT(in <= s32Max); //Value is too large to truncate to s32!
	return (s32) in;
}


template<typename T>
inline bool canCastIntTo(s64 input)
{
	bool result = (input >= minPossibleValue<T>() && input <= maxPossibleValue<T>());

	return result;
}

template<typename T>
inline T truncate(s64 in)
{
	ASSERT(canCastIntTo<T>(in));
	return (T) in;
}

inline f32 clamp01(f32 in)
{
	return clamp(in, 0.0f, 1.0f);
}

u8 clamp01AndMap_u8(f32 in)
{
	return (u8)(clamp01(in) * 255.0f);
}

inline bool equals(f32 a, f32 b, f32 epsilon)
{
	return (abs_f32(a-b) < epsilon);
}

template<typename T>
inline bool equals(T a, T b)
{
	return isMemoryEqual<T>(&a, &b);
}

template<typename T>
inline T clamp(T value, T min, T max)
{
	ASSERT(min <= max); //min > max in clamp()!
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

template<typename T>
inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

template<typename T, typename... Args>
T min(T a, Args... args)
{
	T b = min(args...);
	return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T, typename... Args>
T max(T a, Args... args)
{
	T b = max(args...);
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
	return (T)(a + (b-a)*position);
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
	Array
 **********************************************/

template<typename T>
Array<T> makeArray(s32 count, T *items)
{
	Array<T> result;
	result.count = count;
	result.items = items;

	return result;
}

template<typename T>
inline bool isInitialised(Array<T> *array)
{
	return array->items != null;
}

template<typename T>
inline T& Array<T>::operator[](s32 index)
{
	ASSERT(index >=0 && index < count); //Index out of range!
	return items[index];
}

template<typename T>
inline T *first(Array<T> *array)
{
	ASSERT(array->count > 0); //Index out of range!
	return &array->items[0];
}

template<typename T>
inline T *last(Array<T> *array)
{
	ASSERT(array->count > 0); //Index out of range!
	return &array->items[array->count-1];
}

template<typename T>
void swap(Array<T> *array, s32 indexA, s32 indexB)
{
	T temp = array->items[indexA];
	array->items[indexA] = array->items[indexB];
	array->items[indexB] = temp;
}

template<typename T, typename Comparison>
void sortArrayInternal(Array<T> *array, Comparison compareElements, s32 lowIndex, s32 highIndex)
{
	// Quicksort implementation
	if (lowIndex < highIndex)
	{
		s32 partitionIndex = 0;
		{
			T pivot = array->items[highIndex];
			s32 i = (lowIndex - 1);
			for (s32 j = lowIndex; j < highIndex; j++)
			{
				if (compareElements(array->items[j], pivot))
				{
					i++;
					swap(array, i, j);
				}
			}
			swap(array, i+1, highIndex);
			partitionIndex = i+1;
		}

		sortArrayInternal(array, compareElements, lowIndex, partitionIndex - 1);
		sortArrayInternal(array, compareElements, partitionIndex + 1, highIndex);
	}
}

template<typename T, typename Comparison>
inline void sortArray(Array<T> *array, Comparison compareElements)
{
	sortArrayInternal(array, compareElements, 0, array->count-1);
}

/**********************************************
	Array2
 **********************************************/

template<typename T>
Array2<T> makeArray2(s32 w, s32 h, T *items)
{
	Array2<T> result = {};

	result.w = w;
	result.h = h;
	result.items = items;

	return result;
}

template<typename T>
inline T& Array2<T>::get(s32 x, s32 y)
{
	ASSERT(x >= 0 && x < this->w && y >= 0 && y < this->h);

	return this->items[ (y * this->w) + x ];
}

template<typename T>
inline T Array2<T>::getIfExists(s32 x, s32 y, T defaultValue)
{
	T result = defaultValue;

	if (x >= 0 && x < this->w && y >= 0 && y < this->h)
	{
		result = this->items[ (y * this->w) + x ];
	}

	return result;
}

template<typename T>
inline void Array2<T>::set(s32 x, s32 y, T value)
{
	this->get(x, y) = value;
}

template<typename T>
void fill(Array2<T> *array, T value)
{
	fillMemory<T>(array->items, value, array->w * array->h);
}

template<typename T>
void fillRegion(Array2<T> *array, Rect2I region, T value)
{
	ASSERT(contains(irectXYWH(0,0,array->w, array->h), region));

	for (s32 y = region.y; y < region.y + region.h; y++)
	{
		// Set whole rows at a time
		fillMemory<T>(array->items + (y * array->w) + region.x, value, region.w);
	}
}

/**********************************************
	V2
 **********************************************/

inline V2 v2(f32 x, f32 y)
{
	V2 result;
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

f32 lengthSquaredOf(f32 x, f32 y)
{
	return (f32) (x*x + y * y);
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
	return v2(lhs.x + rhs.x, lhs.y + rhs.y);
}
inline V2 operator+=(V2 &lhs, V2 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V2 operator-(V2 lhs, V2 rhs)
{
	return v2(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline V2 operator-=(V2 &lhs, V2 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V2 operator*(V2 v, f32 s)
{
	return v2(v.x * s, v.y * s);
}
inline V2 operator*=(V2 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V2 operator/(V2 v, f32 s)
{
	return v2(v.x / s, v.y / s);
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
	V2I result;
	result.x = x;
	result.y = y;
	return result;
}

inline V2I v2i(V2 source)
{
	return v2i(floor_s32(source.x), floor_s32(source.y));
}

f32 lengthOf(V2I v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y);
}

f32 lengthOf(s32 x, s32 y)
{
	return (f32) sqrt(x*x + y*y);
}

f32 angleOf(s32 x, s32 y)
{
	return (f32) fmod((atan2(y, x) * radToDeg) + 360.0f, 360.0f);
}

inline V2I operator+(V2I lhs, V2I rhs)
{
	return v2i(lhs.x + rhs.x, lhs.y + rhs.y);
}
inline V2I operator+=(V2I &lhs, V2I rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V2I operator-(V2I lhs, V2I rhs)
{
	return v2i(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline V2I operator-=(V2I &lhs, V2I rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V2I operator*(V2I v, s32 s)
{
	return v2i(v.x * s, v.y * s);
}
inline V2I operator*=(V2I &v, s32 s)
{
	v = v * s;
	return v;
}
inline V2I operator/(V2I v, s32 s)
{
	return v2i(v.x / s, v.y / s);
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
	V3 v;
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
	return v3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}
inline V3 operator+=(V3 &lhs, V3 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V3 operator-(V3 lhs, V3 rhs)
{
	return v3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}
inline V3 operator-=(V3 &lhs, V3 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V3 operator*(V3 v, f32 s)
{
	return v3(v.x * s, v.y * s, v.z * s);
}
inline V3 operator*=(V3 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V3 operator/(V3 v, f32 s)
{
	return v3(v.x / s, v.y / s, v.z / s);
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
	V4 v;
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
	
	V4 v;
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

inline V4 asOpaque(V4 color)
{
	// Colors are always stored with premultiplied alpha, so in order to set the alpha
	// back to 100%, we have to divide by that alpha.

	V4 result = color;

	// If alpha is 0, we can't divide, so just set it
	if (result.a == 0.0f)
	{
		result.a = 1.0f;
	}
	// If alpha is already 1, we don't need to do anything
	// If alpha is > 1, clamp it
	else if (result.a >= 1.0f)
	{
		result.a = 1.0f;
	}
	// Otherwise, divide by the alpha and then make it 1
	else
	{
		result.r = result.r / result.a;
		result.g = result.g / result.a;
		result.b = result.b / result.a;

		result.a = 1.0f;
	}

	return result;
}

inline f32 lengthOf(V4 v)
{
	return (f32) sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline V4 operator+(V4 lhs, V4 rhs)
{
	return v4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}
inline V4 operator+=(V4 &lhs, V4 rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	
	return lhs;
}
inline V4 operator-(V4 lhs, V4 rhs)
{
	return v4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}
inline V4 operator-=(V4 &lhs, V4 rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	lhs.w -= rhs.w;
	
	return lhs;
}
inline V4 operator*(V4 lhs, V4 rhs)
{
	return v4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}
inline V4 operator*=(V4 &lhs, V4 rhs)
{
	lhs.x *= rhs.x;
	lhs.y *= rhs.y;
	lhs.z *= rhs.z;
	lhs.w *= rhs.w;

	return lhs;
}
inline V4 operator*(V4 v, f32 s)
{
	return v4(v.x * s, v.y * s, v.z * s, v.w * s);
}
inline V4 operator*=(V4 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V4 operator/(V4 v, f32 s)
{
	return v4(v.x / s, v.y / s, v.z / s, v.w / s);
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
		case ALIGN_EXPAND_H:  // Same as left
		default:              rect.x = origin.x;                             break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  rect.y = origin.y - round_f32(size.y / 2.0f);  break;
		case ALIGN_BOTTOM:    rect.y = origin.y - size.y;                    break;
		case ALIGN_TOP:       // Top is default
		case ALIGN_EXPAND_V:  // Same as top
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

inline bool overlaps(Rect2 a, Rect2 b)
{
	return (a.x < b.x + b.w)
		&& (b.x < a.x + a.w)
		&& (a.y < b.y + b.h)
		&& (b.y < a.y + a.h);
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
	return abs_f32(rect.w * rect.h);
}

inline V2 alignWithinRectangle(Rect2 bounds, u32 alignment, f32 padding)
{
	V2 result;

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  result.x = bounds.x + bounds.w / 2.0f;     break;
		case ALIGN_RIGHT:     result.x = bounds.x + bounds.w - padding;  break;
		case ALIGN_LEFT:      // Left is default
		case ALIGN_EXPAND_H:  // Meaningless here so default to left
		default:              result.x = bounds.x + padding;             break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  result.y = bounds.y + bounds.h / 2.0f;     break;
		case ALIGN_BOTTOM:    result.y = bounds.y + bounds.h - padding;  break;
		case ALIGN_TOP:       // Top is default
		case ALIGN_EXPAND_V:  // Meaningless here so default to top
		default:              result.y = bounds.y + padding;             break;
	}

	return result;
}

/**********************************************
	Rectangle (int)
 **********************************************/

inline Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h)
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
inline Rect2I irectInfinity()
{
	return irectXYWH(s32Min / 2, s32Min / 2, s32Max, s32Max);
}

// Rectangle that is guaranteed to not contain anything, because it's inside-out
inline Rect2I irectNegativeInfinity()
{
	return irectXYWH(s32Max, s32Max, s32Min, s32Min);
}


inline Rect2I irectPosSize(V2I position, V2I size)
{
	Rect2I rect;
	rect.pos = position;
	rect.size = size;
	return rect;
}

inline Rect2I irectCentreSize(s32 centreX, s32 centreY, s32 sizeX, s32 sizeY)
{
	return irectXYWH(centreX - (sizeX / 2), centreY - (sizeY / 2), sizeX, sizeY); 
}

inline Rect2I irectCentreSize(V2I position, V2I size)
{
	return irectPosSize(position - (size / 2), size);
}

inline Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax)
{
	return irectXYWH(xMin, yMin, (1+xMax-xMin), (1+yMax-yMin));
}

inline Rect2I irectAligned(s32 originX, s32 originY, s32 w, s32 h, u32 alignment)
{
	Rect2I rect = {};
	rect.w = w;
	rect.h = h;

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  rect.x = originX - (w / 2);  break;
		case ALIGN_RIGHT:     rect.x = originX - w;        break;
		case ALIGN_LEFT:      // Left is default
		case ALIGN_EXPAND_H:  // Meaningless here so default to left
		default:              rect.x = originX;                 break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  rect.y = originY - (h / 2);  break;
		case ALIGN_BOTTOM:    rect.y = originY - h;        break;
		case ALIGN_TOP:       // Top is default
		case ALIGN_EXPAND_V:  // Meaningless here so default to top
		default:              rect.y = originY;                 break;
	}

	return rect;
}

inline Rect2I irectAligned(V2I origin, V2I size, u32 alignment)
{
	return irectAligned(origin.x, origin.y, size.x, size.y, alignment);
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

inline bool contains(Rect2I rect, V2 pos)
{
	return (pos.x >= rect.x)
		&& (pos.x < rect.x + rect.w)
		&& (pos.y >= rect.y)
		&& (pos.y < rect.y + rect.h);
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

inline Rect2I shrink(Rect2I rect, s32 radius)
{
	s32 doubleRadius = radius * 2;
	if ((rect.w >= doubleRadius) && (rect.h >= doubleRadius))
	{
		return irectXYWH(
			rect.x + radius,
			rect.y + radius,
			rect.w - doubleRadius,
			rect.h - doubleRadius
		);
	}
	else
	{
		V2 centre = centreOf(rect);
		return irectCentreSize((s32)centre.x, (s32)centre.y,
								max(rect.w - doubleRadius, 0), max(rect.h - doubleRadius, 0));
	}
}

inline Rect2I shrink(Rect2I rect, s32 top, s32 right, s32 bottom, s32 left)
{
	s32 dWidth = left + right;
	s32 dHeight = top + bottom;

	Rect2I result = irectXYWH(
		rect.x + left,
		rect.y + top,
		rect.w - dWidth,
		rect.h - dHeight
	);

	// NB: We do this calculation differently from the simpler shrink(rect, radius), because
	// the shrink amount might be different from one side to the other, which means the new
	// centre is not the same as the old centre!
	// So, we create the new, possibly inside-out rectangle, and then set its dimensions
	// to 0 if they are negative, while maintaining the centre point.

	V2 centre = centreOf(result);

	if (result.w < 0)
	{
		result.x = (s32) centre.x;
		result.w = 0;
	}

	if (result.h < 0)
	{
		result.y = (s32) centre.y;
		result.h = 0;
	}

	return result;
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

inline Rect2I unionOf(Rect2I a, Rect2I b)
{
	s32 minX = min(a.x, b.x);
	s32 minY = min(a.y, b.y);
	s32 maxX = max(a.x+a.w-1, b.x+b.w-1);
	s32 maxY = max(a.y+a.h-1, b.y+b.h-1);

	return irectMinMax(minX, minY, maxX, maxY);
}

inline V2 centreOf(Rect2I rect)
{
	return v2(rect.x + rect.w/2.0f, rect.y + rect.h/2.0f);
}

inline s32 areaOf(Rect2I rect)
{
	return abs_s32(rect.w * rect.h);
}

inline Rect2I centreWithin(Rect2I outer, Rect2I inner)
{
	Rect2I result = inner;

	result.x = outer.x - ((result.w - outer.w) / 2);
	result.y = outer.y - ((result.h - outer.h) / 2);

	return result;
}

inline V2I alignWithinRectangle(Rect2I bounds, u32 alignment, s32 padding)
{
	V2I result;

	switch (alignment & ALIGN_H)
	{
		case ALIGN_H_CENTRE:  result.x = bounds.x + (bounds.w / 2);      break;
		case ALIGN_RIGHT:     result.x = bounds.x + bounds.w - padding;  break;
		case ALIGN_LEFT:      // Left is default
		case ALIGN_EXPAND_H:  // Meaningless here so default to left
		default:              result.x = bounds.x + padding;             break;
	}

	switch (alignment & ALIGN_V)
	{
		case ALIGN_V_CENTRE:  result.y = bounds.y + (bounds.h / 2);      break;
		case ALIGN_BOTTOM:    result.y = bounds.y + bounds.h - padding;  break;
		case ALIGN_TOP:       // Top is default
		case ALIGN_EXPAND_V:  // Meaningless here so default to top
		default:              result.y = bounds.y + padding;             break;
	}

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
	Matrix4 result;

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

// Matrix multiplication
inline Matrix4 operator*(Matrix4 a, Matrix4 b) {
	Matrix4 result;

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
	V4 result;

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

template<typename T>
inline String typeNameOf()
{
	return makeString( typeid(T).name() );
}
