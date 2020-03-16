#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 offset;
layout (location = 4) in float scalar;

uniform mat4 view;
uniform mat4 proj;

out vec4 color;

void main() {
	vec4 off = offset *  1000.0f;
	off.w = 0.0f;
	color = vec4(scalar, 0, 0, 1);
	gl_Position = proj * view * (position + off);
}
