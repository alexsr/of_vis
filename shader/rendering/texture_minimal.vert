#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 pass_uv;

void main() {
    pass_uv = uv;
    gl_Position = proj * view * model * position;
}
