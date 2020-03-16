#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (std430, binding = COLOR_BUFFER) buffer color_buffer {
    vec4 in_color[];
};

out vec4 pass_pos_g;
out vec3 pass_normal_g;
out vec4 pass_color_g;

void main() {
    pass_pos_g = view * model * position;
    pass_normal_g = vec3(transpose(inverse(view * model)) * normal);
    pass_color_g = in_color[gl_VertexID];
    gl_Position = proj * pass_pos_g;
}
