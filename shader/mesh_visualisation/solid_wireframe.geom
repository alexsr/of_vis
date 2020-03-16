#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

vec3 calc_normal(vec4 p1, vec4 p2, vec4 p3) {
    return cross(vec3(p2 - p1), vec3(p3 - p1));
}

in vec4 pass_position[];
in vec3 pass_color_gs[];

out vec3 pass_color;
noperspective out vec3 dist_to_edge;

uniform vec2 win_res;

void main() {
    vec2 vert_a = win_res * pass_position[0].xy / pass_position[0].w;
    vec2 vert_b = win_res * pass_position[1].xy / pass_position[1].w;
    vec2 vert_c = win_res * pass_position[2].xy / pass_position[2].w;
    vec2 edge_ab = vert_b - vert_a;
    vec2 edge_bc = vert_c - vert_b;
    vec2 edge_ca = vert_a - vert_c;
    float area = abs(edge_ca.y * edge_ab.x - edge_ca.x * edge_ab.y);
    float height_to_a = area / length(edge_bc);
    float height_to_b = area / length(edge_ca);
    float height_to_c = area / length(edge_ab);
    gl_Position = gl_in[0].gl_Position;
    pass_color = pass_color_gs[0];
    dist_to_edge = vec3(height_to_a, 0, 0);
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    pass_color = pass_color_gs[1];
    dist_to_edge = vec3(0, height_to_b, 0);
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    pass_color = pass_color_gs[2];
    dist_to_edge = vec3(0, 0, height_to_c);
    EmitVertex();
    EndPrimitive();
}