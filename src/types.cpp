#pragma once

/**********************************************
    Array
 **********************************************/

template<typename T>
Array<T> makeArray(s32 capacity, T* items, s32 count)
{
    Array<T> result;
    result.capacity = capacity;
    result.count = count;
    result.items = items;

    return result;
}

template<typename T>
inline Array<T> makeEmptyArray()
{
    return makeArray<T>(0, nullptr);
}

template<typename T>
inline T& Array<T>::operator[](s32 index)
{
    ASSERT(index >= 0 && index < count); // Index out of range!
    return items[index];
}

template<typename T>
inline T* Array<T>::get(s32 index)
{
    ASSERT(index >= 0 && index < count); // Index out of range!
    return &items[index];
}

template<typename T>
inline T* Array<T>::first()
{
    ASSERT(count > 0); // Index out of range!
    return &items[0];
}

template<typename T>
inline T* Array<T>::last()
{
    ASSERT(count > 0); // Index out of range!
    return &items[count - 1];
}

template<typename T>
inline T* Array<T>::append()
{
    ASSERT(count < capacity);

    T* result = items + count++;

    return result;
}

template<typename T>
inline T* Array<T>::append(T item)
{
    T* result = append();
    *result = item;
    return result;
}

template<typename T>
inline bool Array<T>::isInitialised()
{
    return items != nullptr;
}

template<typename T>
bool Array<T>::isEmpty()
{
    return count == 0;
}

template<typename T>
bool Array<T>::hasRoom()
{
    return count < capacity;
}

template<typename T>
void Array<T>::swap(s32 indexA, s32 indexB)
{
    T temp = items[indexA];
    items[indexA] = items[indexB];
    items[indexB] = temp;
}

template<typename T>
template<typename Comparison>
inline void Array<T>::sort(Comparison compareElements)
{
    DEBUG_FUNCTION();

    sortInternal(compareElements, 0, count - 1);
}

template<typename T>
template<typename Comparison>
void Array<T>::sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex)
{
    // Quicksort implementation
    if (lowIndex < highIndex) {
        s32 partitionIndex = 0;
        {
            T pivot = items[highIndex];
            s32 i = (lowIndex - 1);
            for (s32 j = lowIndex; j < highIndex; j++) {
                if (compareElements(items[j], pivot)) {
                    i++;
                    swap(i, j);
                }
            }
            swap(i + 1, highIndex);
            partitionIndex = i + 1;
        }

        sortInternal(compareElements, lowIndex, partitionIndex - 1);
        sortInternal(compareElements, partitionIndex + 1, highIndex);
    }
}

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
