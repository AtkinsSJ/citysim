#pragma once

template<typename T>
inline bool equals(T a, T b)
{
    return isMemoryEqual<T>(&a, &b);
}

template<typename T>
inline String typeNameOf()
{
    return makeString(typeid(T).name());
}

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
    return makeArray<T>(0, null);
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
    return items != null;
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
    m.v[0][0] = 2.0f / (right - left);
    m.v[1][1] = 2.0f / (top - bottom);
    m.v[2][2] = -2.0f / (farClip - nearClip);

    m.v[3][0] = -(right + left) / (right - left);
    m.v[3][1] = -(top + bottom) / (top - bottom);
    m.v[3][2] = -(farClip + nearClip) / (farClip - nearClip);
    m.v[3][3] = 1.0f;

    return m;
}

Matrix4 inverse(Matrix4* source)
{
    DEBUG_FUNCTION();

    // Shamelessly copied from http://stackoverflow.com/a/1148405/1178345
    // This is complicated! *_*
    Matrix4 result;

    result.flat[0] = source->flat[5] * source->flat[10] * source->flat[15] - source->flat[5] * source->flat[11] * source->flat[14] - source->flat[9] * source->flat[6] * source->flat[15] + source->flat[9] * source->flat[7] * source->flat[14] + source->flat[13] * source->flat[6] * source->flat[11] - source->flat[13] * source->flat[7] * source->flat[10];

    result.flat[4] = -source->flat[4] * source->flat[10] * source->flat[15] + source->flat[4] * source->flat[11] * source->flat[14] + source->flat[8] * source->flat[6] * source->flat[15] - source->flat[8] * source->flat[7] * source->flat[14] - source->flat[12] * source->flat[6] * source->flat[11] + source->flat[12] * source->flat[7] * source->flat[10];

    result.flat[8] = source->flat[4] * source->flat[9] * source->flat[15] - source->flat[4] * source->flat[11] * source->flat[13] - source->flat[8] * source->flat[5] * source->flat[15] + source->flat[8] * source->flat[7] * source->flat[13] + source->flat[12] * source->flat[5] * source->flat[11] - source->flat[12] * source->flat[7] * source->flat[9];

    result.flat[12] = -source->flat[4] * source->flat[9] * source->flat[14] + source->flat[4] * source->flat[10] * source->flat[13] + source->flat[8] * source->flat[5] * source->flat[14] - source->flat[8] * source->flat[6] * source->flat[13] - source->flat[12] * source->flat[5] * source->flat[10] + source->flat[12] * source->flat[6] * source->flat[9];

    result.flat[1] = -source->flat[1] * source->flat[10] * source->flat[15] + source->flat[1] * source->flat[11] * source->flat[14] + source->flat[9] * source->flat[2] * source->flat[15] - source->flat[9] * source->flat[3] * source->flat[14] - source->flat[13] * source->flat[2] * source->flat[11] + source->flat[13] * source->flat[3] * source->flat[10];

    result.flat[5] = source->flat[0] * source->flat[10] * source->flat[15] - source->flat[0] * source->flat[11] * source->flat[14] - source->flat[8] * source->flat[2] * source->flat[15] + source->flat[8] * source->flat[3] * source->flat[14] + source->flat[12] * source->flat[2] * source->flat[11] - source->flat[12] * source->flat[3] * source->flat[10];

    result.flat[9] = -source->flat[0] * source->flat[9] * source->flat[15] + source->flat[0] * source->flat[11] * source->flat[13] + source->flat[8] * source->flat[1] * source->flat[15] - source->flat[8] * source->flat[3] * source->flat[13] - source->flat[12] * source->flat[1] * source->flat[11] + source->flat[12] * source->flat[3] * source->flat[9];

    result.flat[13] = source->flat[0] * source->flat[9] * source->flat[14] - source->flat[0] * source->flat[10] * source->flat[13] - source->flat[8] * source->flat[1] * source->flat[14] + source->flat[8] * source->flat[2] * source->flat[13] + source->flat[12] * source->flat[1] * source->flat[10] - source->flat[12] * source->flat[2] * source->flat[9];

    result.flat[2] = source->flat[1] * source->flat[6] * source->flat[15] - source->flat[1] * source->flat[7] * source->flat[14] - source->flat[5] * source->flat[2] * source->flat[15] + source->flat[5] * source->flat[3] * source->flat[14] + source->flat[13] * source->flat[2] * source->flat[7] - source->flat[13] * source->flat[3] * source->flat[6];

    result.flat[6] = -source->flat[0] * source->flat[6] * source->flat[15] + source->flat[0] * source->flat[7] * source->flat[14] + source->flat[4] * source->flat[2] * source->flat[15] - source->flat[4] * source->flat[3] * source->flat[14] - source->flat[12] * source->flat[2] * source->flat[7] + source->flat[12] * source->flat[3] * source->flat[6];

    result.flat[10] = source->flat[0] * source->flat[5] * source->flat[15] - source->flat[0] * source->flat[7] * source->flat[13] - source->flat[4] * source->flat[1] * source->flat[15] + source->flat[4] * source->flat[3] * source->flat[13] + source->flat[12] * source->flat[1] * source->flat[7] - source->flat[12] * source->flat[3] * source->flat[5];

    result.flat[14] = -source->flat[0] * source->flat[5] * source->flat[14] + source->flat[0] * source->flat[6] * source->flat[13] + source->flat[4] * source->flat[1] * source->flat[14] - source->flat[4] * source->flat[2] * source->flat[13] - source->flat[12] * source->flat[1] * source->flat[6] + source->flat[12] * source->flat[2] * source->flat[5];

    result.flat[3] = -source->flat[1] * source->flat[6] * source->flat[11] + source->flat[1] * source->flat[7] * source->flat[10] + source->flat[5] * source->flat[2] * source->flat[11] - source->flat[5] * source->flat[3] * source->flat[10] - source->flat[9] * source->flat[2] * source->flat[7] + source->flat[9] * source->flat[3] * source->flat[6];

    result.flat[7] = source->flat[0] * source->flat[6] * source->flat[11] - source->flat[0] * source->flat[7] * source->flat[10] - source->flat[4] * source->flat[2] * source->flat[11] + source->flat[4] * source->flat[3] * source->flat[10] + source->flat[8] * source->flat[2] * source->flat[7] - source->flat[8] * source->flat[3] * source->flat[6];

    result.flat[11] = -source->flat[0] * source->flat[5] * source->flat[11] + source->flat[0] * source->flat[7] * source->flat[9] + source->flat[4] * source->flat[1] * source->flat[11] - source->flat[4] * source->flat[3] * source->flat[9] - source->flat[8] * source->flat[1] * source->flat[7] + source->flat[8] * source->flat[3] * source->flat[5];

    result.flat[15] = source->flat[0] * source->flat[5] * source->flat[10] - source->flat[0] * source->flat[6] * source->flat[9] - source->flat[4] * source->flat[1] * source->flat[10] + source->flat[4] * source->flat[2] * source->flat[9] + source->flat[8] * source->flat[1] * source->flat[6] - source->flat[8] * source->flat[2] * source->flat[5];

    f32 det = source->flat[0] * result.flat[0] + source->flat[1] * result.flat[4] + source->flat[2] * result.flat[8] + source->flat[3] * result.flat[12];
    if (det != 0) {
        det = 1.0f / det;
        for (int i = 0; i < 16; i++) {
            result.flat[i] *= det;
        }
    }

    return result;
}

void translate(Matrix4* matrix, V3 translation)
{
    matrix->v[3][0] += translation.x;
    matrix->v[3][1] += translation.y;
    matrix->v[3][2] += translation.z;
}

void scale(Matrix4* matrix, V3 scale)
{
    matrix->v[0][0] *= scale.x;
    matrix->v[1][1] *= scale.y;
    matrix->v[2][2] *= scale.z;
}

void rotateZ(Matrix4* matrix, f32 radians)
{
    Matrix4 rotation = identityMatrix4();
    f32 c = (f32)cos(radians);
    f32 s = (f32)sin(radians);
    rotation.v[0][0] = c;
    rotation.v[0][1] = s;
    rotation.v[1][0] = -s;
    rotation.v[1][1] = c;

    (*matrix) *= rotation;
}

// Matrix multiplication
inline Matrix4 operator*(Matrix4 a, Matrix4 b)
{
    Matrix4 result;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {

            result.v[col][row] = (a.v[0][row] * b.v[col][0])
                + (a.v[1][row] * b.v[col][1])
                + (a.v[2][row] * b.v[col][2])
                + (a.v[3][row] * b.v[col][3]);
        }
    }

    return result;
}

inline Matrix4 operator*=(Matrix4& a, Matrix4 b)
{
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
