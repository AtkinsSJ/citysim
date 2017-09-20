in vec3 aPosition;
in vec4 aColor;
in vec2 aUV;

out vec4 vColor;
out vec2 vUV;

uniform mat4 uProjectionMatrix;

void main() {
	gl_Position = uProjectionMatrix * vec4( aPosition.xyz, 1 );
	vColor = aColor;
	vUV = aUV;
}