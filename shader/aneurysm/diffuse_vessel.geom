#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec4 pass_pos_g[];
in vec3 pass_normal_g[];
in vec4 pass_color_g[];

out vec4 pass_pos;
out vec3 pass_normal;
flat out vec4 pass_color;

uniform vec4 anuerysm_color;

void main() {
    vec4 color_v0 = pass_color_g[0];
    vec4 color_v1 = pass_color_g[1];
    vec4 color_v2 = pass_color_g[2];
    vec4 color = anuerysm_color;
    if (color_v0 == color_v1 && color_v0 == color_v2) {
        color = color_v0;
    }
    gl_Position = gl_in[0].gl_Position;
    pass_normal = pass_normal_g[0];
    pass_color = color;
    pass_pos = pass_pos_g[0];
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    pass_normal = pass_normal_g[1];
    pass_color = color;
    pass_pos = pass_pos_g[1];
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    pass_normal = pass_normal_g[2];
    pass_color = color;
    pass_pos = pass_pos_g[2];
    EmitVertex();
    EndPrimitive();
}
