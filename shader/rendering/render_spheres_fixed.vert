#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

uniform mat4 view;
uniform mat4 proj;

out vec2 pass_uv;
flat out vec4 pass_pos;
flat out float pass_radius;
flat out vec4 pass_color;

void main() {
    pass_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    pass_pos = view * position;
    pass_radius = 0.01f;
	pass_color = color;
    gl_Position = proj * (pass_pos + vec4(pass_radius * (pass_uv * 2.0f + -1.0f), 0.0f, 0.0f));
}
