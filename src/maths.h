#pragma once

const f32 PI32 = 3.14159265358979323846f;
const f32 radToDeg = 180.0f / PI32;
const f32 degToRad = PI32 / 180.0f;

// Standard rounding functions return doubles, so here's some int ones.
s32 round_s32(f32 in);
s32 floor_s32(f32 in);
s32 ceil_s32(f32 in);
s32 abs_s32(s32 in);
f32 round_f32(f32 in);
f32 floor_f32(f32 in);
f32 ceil_f32(f32 in);
f32 sqrt_f32(f32 in);
f32 abs_f32(f32 in);
f32 fraction_f32(f32 in);
f32 clamp01(f32 in);
f32 sin32(f32 radians);
f32 cos32(f32 radians);
f32 tan32(f32 radians);

s32 divideCeil(s32 numerator, s32 denominator);

s32 truncate32(s64 in);

template<typename T>
bool canCastIntTo(s64 input);

template<typename T>
T truncate(s64 in);

u8 clamp01AndMap_u8(f32 in);

template<typename T>
T clamp(T value, T min, T max);

template<typename T>
T min(T a, T b);

template<typename T>
inline T min(T a) { return a; }

template<typename T, typename... Args>
T min(T a, Args... args);

template<typename T>
T max(T a, T b);

template<typename T>
inline T max(T a) { return a; }

template<typename T, typename... Args>
T max(T a, Args... args);

template<typename T>
T wrap(T value, T max);

template<typename T>
T lerp(T a, T b, f32 position);

template<typename T>
T approach(T currentValue, T targetValue, T distance);

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
s32 manhattanDistance(Rect2I rect, V2I point);
s32 manhattanDistance(Rect2I a, Rect2I b);

bool equals(f32 a, f32 b, f32 epsilon);

//
// All this mess is just so we can access a type's min/max values from a template.
//
template<typename T> const inline T minPossibleValue();
template<typename T> const inline T maxPossibleValue();

template<> const inline u8  minPossibleValue<u8>()  { return 0;      }
template<> const inline u8  maxPossibleValue<u8>()  { return u8Max;  }
template<> const inline u16 minPossibleValue<u16>() { return 0;      }
template<> const inline u16 maxPossibleValue<u16>() { return u16Max; }
template<> const inline u32 minPossibleValue<u32>() { return 0;      }
template<> const inline u32 maxPossibleValue<u32>() { return u32Max; }
template<> const inline u64 minPossibleValue<u64>() { return 0;      }
template<> const inline u64 maxPossibleValue<u64>() { return u64Max; }

template<> const inline s8  minPossibleValue<s8>()  { return s8Min;  }
template<> const inline s8  maxPossibleValue<s8>()  { return s8Max;  }
template<> const inline s16 minPossibleValue<s16>() { return s16Min; }
template<> const inline s16 maxPossibleValue<s16>() { return s16Max; }
template<> const inline s32 minPossibleValue<s32>() { return s32Min; }
template<> const inline s32 maxPossibleValue<s32>() { return s32Max; }
template<> const inline s64 minPossibleValue<s64>() { return s64Min; }
template<> const inline s64 maxPossibleValue<s64>() { return s64Max; }

template<> const inline f32 minPossibleValue<f32>() { return f32Min; }
template<> const inline f32 maxPossibleValue<f32>() { return f32Max; }
template<> const inline f64 minPossibleValue<f64>() { return f64Min; }
template<> const inline f64 maxPossibleValue<f64>() { return f64Max; }
