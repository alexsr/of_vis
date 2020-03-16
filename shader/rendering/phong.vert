#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec4 pass_pos;
out vec3 pass_normal;

void main() {
    pass_pos = view * model * position;
    pass_normal = vec3(transpose(inverse(view * model)) * normal);
    gl_Position = proj * pass_pos;
}
