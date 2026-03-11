/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>

//
// Matrix4
//
// Matrices! Pretty sure I'm only going to use this for the camera projection,
// so certain things are missing - notably, we can only rotate in the z-axis
// because (for now) the game will be in 2D.
//
// TODO: When I move to 3D, implement full rotation of matrices!
//
struct Matrix4 {
    union {
        float v[4][4]; // Column-major order, so [COLUMN][ROW]
        float flat[4 * 4];
    };
};

Matrix4 identityMatrix4();
Matrix4 orthographicMatrix4(float left, float right, float top, float bottom, float nearClip, float farClip);
Matrix4 inverse(Matrix4* source);
void translate(Matrix4* matrix, V3 translation);
void scale(Matrix4* matrix, V3 scale);
void rotateZ(Matrix4* matrix, float radians);

Matrix4 operator*(Matrix4 a, Matrix4 b);
Matrix4 operator*=(Matrix4& a, Matrix4 b);
V4 operator*(Matrix4 m, V4 v);
