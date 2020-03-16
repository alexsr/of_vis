#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in float radius;

uniform mat4 view;
uniform mat4 proj;

out vec2 pass_uv;
flat out vec4 pass_pos;
flat out float pass_radius;

void main() {
    pass_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    pass_pos = view * position;
    pass_radius = radius;
    gl_Position = proj * (pass_pos + vec4(pass_radius * (pass_uv * 2.0f + -1.0f), 0.0f, 0.0f));
}
