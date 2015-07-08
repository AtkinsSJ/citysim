#pragma once

struct matrix4 {
	union {
		real32 values[4][4]; // Column-major order, so [COLUMN][ROW]
		GLfloat flatValues[4*4];
	};
};

matrix4 identityMatrix4() {
	matrix4 m = {};
	m.values[0][0] = 1;
	m.values[1][1] = 1;
	m.values[2][2] = 1;
	m.values[3][3] = 1;

	return m;
}