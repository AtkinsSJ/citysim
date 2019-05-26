#pragma once

Matrix4 identityMatrix4()
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
