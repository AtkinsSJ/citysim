#version 150

in vec2 aPosition;
in vec4 aColor;
in vec2 aUV;

out vec4 vColor;
out vec2 vUV;

uniform mat4 uProjectionMatrix;

void main()
{
	gl_Position = uProjectionMatrix * vec4(aPosition.xy, 0, 1);
	vColor = aColor;
	vUV = aUV;
}

$

#version 150

uniform sampler2D uTexture;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

void main()
{
	fragColor = vColor;

	vec4 texel = texture(uTexture, vUV);
	fragColor *= texel;
}
