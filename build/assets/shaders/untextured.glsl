#version 150

in vec3 aPosition;
in vec4 aColor;

out vec4 vColor;

uniform mat4 uProjectionMatrix;

void main()
{
	gl_Position = uProjectionMatrix * vec4(aPosition.xyz, 1);
	vColor = aColor;
}

$

#version 150

in vec4 vColor;

out vec4 fragColor;

void main()
{
	fragColor = vColor;
}
