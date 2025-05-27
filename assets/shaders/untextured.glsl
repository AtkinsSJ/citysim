#version 150

in vec2 aPosition;
in vec4 aColor;

out vec4 vColor;

uniform mat4 uProjectionMatrix;

void main()
{
	gl_Position = uProjectionMatrix * vec4(aPosition.xy, 0, 1);
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
