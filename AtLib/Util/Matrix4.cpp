/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Matrix4.h"
#include <Util/Maths.h>
#include <Util/Vector.h>

Matrix4 identityMatrix4()
{
    Matrix4 m = {};
    m.v[0][0] = 1;
    m.v[1][1] = 1;
    m.v[2][2] = 1;
    m.v[3][3] = 1;

    return m;
}

Matrix4 orthographicMatrix4(float left, float right, float top, float bottom, float nearClip, float farClip)
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

    float det = source->flat[0] * result.flat[0] + source->flat[1] * result.flat[4] + source->flat[2] * result.flat[8] + source->flat[3] * result.flat[12];
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

void rotateZ(Matrix4* matrix, float radians)
{
    Matrix4 rotation = identityMatrix4();
    float c = cos32(radians);
    float s = sin32(radians);
    rotation.v[0][0] = c;
    rotation.v[0][1] = s;
    rotation.v[1][0] = -s;
    rotation.v[1][1] = c;

    (*matrix) *= rotation;
}

// Matrix multiplication
Matrix4 operator*(Matrix4 a, Matrix4 b)
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

Matrix4 operator*=(Matrix4& a, Matrix4 b)
{
    a = a * b;
    return a;
}

V4 operator*(Matrix4 m, V4 v)
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
