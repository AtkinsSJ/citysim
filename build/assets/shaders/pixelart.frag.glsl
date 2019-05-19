#version 150

uniform sampler2D uTexture;
uniform float uScale;

in vec4 vColor;
in vec2 vUV;

out vec4 fragColor;

void main() {
	fragColor = vColor;

	// https://hero.handmade.network/forums/code-discussion/t/3618-scaling_in_bitmap_based_games#17223
	// https://www.shadertoy.com/view/MlB3D3
	// The basic gist of that is, get the pixel coordinates,
	// then figure out whether we're safely inside a pixel, or on the border.
	// So, we only blend the single border pixel.

	vec2 textureSize = textureSize(uTexture, 0);

	vec2 pixel = vUV * textureSize;

	vec2 uv = floor(pixel) + 0.5;
	uv += 1.0 - clamp((1.0 - fract(pixel)) * uScale, 0.0, 1.0);

	uv /= textureSize;

	vec4 texel = texture(uTexture, uv);
	fragColor *= texel;
}