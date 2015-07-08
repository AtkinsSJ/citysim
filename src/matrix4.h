#pragma once

/*
	Matrices! Pretty sure I'm only going to use this for the camera projection,
	so certain things are missing - notably, we can only rotate in the z-axis
	because (for now) the game will be in 2D.

	TODO: When I move to 3D, implement full rotation of matrices!
*/

struct Matrix4 {
	union {
		GLfloat v[4][4]; // Column-major order, so [COLUMN][ROW]
		GLfloat flat[4*4];
	};
};

Matrix4 identityMatrix4() {
	Matrix4 m = {};
	m.v[0][0] = 1;
	m.v[1][1] = 1;
	m.v[2][2] = 1;
	m.v[3][3] = 1;

	return m;
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
inline Matrix4 operator*(Matrix4 m, GLfloat f) {
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

void translate(Matrix4 *matrix, V3 translation) {
	matrix->v[3][0] += translation.x;
	matrix->v[3][1] += translation.y;
	matrix->v[3][2] += translation.z;
}

void scale(Matrix4 *matrix, V3 scale) {
	matrix->v[0][0] *= scale.x;
	matrix->v[1][1] *= scale.y;
	matrix->v[2][2] *= scale.z;
}

void rotateZ(Matrix4 *matrix, real32 radians) {
	Matrix4 rotation = identityMatrix4();
	real32 c = cos(radians),
		   s = sin(radians);
	rotation.v[0][0] = c;
	rotation.v[0][1] = s;
	rotation.v[1][0] = -s;
	rotation.v[1][1] = c;

	(*matrix) *= rotation;
}