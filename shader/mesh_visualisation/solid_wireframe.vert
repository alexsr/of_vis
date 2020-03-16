#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec4 pass_position;
out vec3 pass_color_gs;

void main() {
    pass_color_gs = vec3(transpose(inverse(view * model)) * normal);
    pass_position = proj * view * model * position;
    gl_Position = proj * view * model * position;
}
