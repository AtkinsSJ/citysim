#pragma once

/**********************************************
    Array2
 **********************************************/

template<typename T>
Array2<T> makeArray2(s32 w, s32 h, T* items)
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

    return this->items[(y * this->w) + x];
}

template<typename T>
inline T Array2<T>::getIfExists(s32 x, s32 y, T defaultValue)
{
    T result = defaultValue;

    if (x >= 0 && x < this->w && y >= 0 && y < this->h) {
        result = this->items[(y * this->w) + x];
    }

    return result;
}

template<typename T>
inline void Array2<T>::set(s32 x, s32 y, T value)
{
    this->get(x, y) = value;
}

template<typename T>
void fill(Array2<T>* array, T value)
{
    fillMemory<T>(array->items, value, array->w * array->h);
}

template<typename T>
void fillRegion(Array2<T>* array, Rect2I region, T value)
{
    ASSERT(contains(irectXYWH(0, 0, array->w, array->h), region));

    for (s32 y = region.y; y < region.y + region.h; y++) {
        // Set whole rows at a time
        fillMemory<T>(array->items + (y * array->w) + region.x, value, region.w);
    }
}

/**********************************************
    Colours
 **********************************************/

inline V4 color255(u8 r, u8 g, u8 b, u8 a)
{
    static f32 const inv255 = 1.0f / 255.0f;

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
    return v4(1.0f, 1.0f, 1.0f, 1.0f);
}

inline V4 asOpaque(V4 color)
{
    // Colors are always stored with premultiplied alpha, so in order to set the alpha
    // back to 100%, we have to divide by that alpha.

    V4 result = color;

    // If alpha is 0, we can't divide, so just set it
    if (result.a == 0.0f) {
        result.a = 1.0f;
    }
    // If alpha is already 1, we don't need to do anything
    // If alpha is > 1, clamp it
    else if (result.a >= 1.0f) {
        result.a = 1.0f;
    }
    // Otherwise, divide by the alpha and then make it 1
    else {
        result.r = result.r / result.a;
        result.g = result.g / result.a;
        result.b = result.b / result.a;

        result.a = 1.0f;
    }

    return result;
}
