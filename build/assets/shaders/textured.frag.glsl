#version 150

uniform sampler2D uTexture;
uniform float uScale;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

void main() {
	fragColor = vColor;

	vec4 texel = texture(uTexture, vUV);
	fragColor *= texel;
}