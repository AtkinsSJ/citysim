#pragma once

/*
	Matrices! Pretty sure I'm only going to use this for the camera projection,
	so certain things are missing - notably, we can only rotate in the z-axis
	because (for now) the game will be in 2D.

	TODO: When I move to 3D, implement full rotation of matrices!
*/
Matrix4 identityMatrix4();
Matrix4 orthographicMatrix4(f32 left, f32 right, f32 top, f32 bottom, f32 nearClip, f32 farClip);
Matrix4 inverse(Matrix4 *source);
void translate(Matrix4 *matrix, V3 translation);
void scale(Matrix4 *matrix, V3 scale);
void rotateZ(Matrix4 *matrix, f32 radians);

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
