#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 offset;

uniform mat4 view;
uniform mat4 proj;

void main() {
    vec4 off = offset;
    off.w = 0.0f;
    gl_Position = proj * view * (position + off);
}
