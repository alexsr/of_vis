#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

vec3 calc_normal(vec4 p1, vec4 p2, vec4 p3) {
    return cross(vec3(p2 - p1), vec3(p3 - p1));
}

in vec4 pass_position[];
in vec3 pass_normal_gs[];
in float selected[];

out vec3 pass_normal;
out vec3 color;
noperspective out vec3 dist_to_edge;

uniform vec2 win_res;
uniform vec3 wireframe_color;
uniform vec3 select_wireframe_color;
uniform float line_width;

void main() {
    vec3 out_color = wireframe_color;
    if (selected[0] > 0 && selected[1] > 0 && selected[2] > 0) {
        out_color = select_wireframe_color;
    }
    vec2 vert_a = win_res * pass_position[0].xy / pass_position[0].w;
    vec2 vert_b = win_res * pass_position[1].xy / pass_position[1].w;
    vec2 vert_c = win_res * pass_position[2].xy / pass_position[2].w;
    vec2 edge_ab = vert_b - vert_a; // p1 - p0 v2
    vec2 edge_bc = vert_c - vert_b; // p2 - p1 v0
    vec2 edge_ac = vert_c - vert_a; // p2 - p0 v1
    float area = abs(edge_ac.x * edge_ab.y - edge_ac.y * edge_ab.x);
    float height_to_a = area / length(edge_bc);
    float height_to_b = area / length(edge_ac);
    float height_to_c = area / length(edge_ab);
    gl_Position = gl_in[0].gl_Position;
    pass_normal = pass_normal_gs[0];
    color = out_color;
    dist_to_edge = vec3(height_to_a, 0, 0);
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    pass_normal = pass_normal_gs[1];
    color = out_color;
    dist_to_edge = vec3(0, height_to_b, 0);
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    pass_normal = pass_normal_gs[2];
    color = out_color;
    dist_to_edge = vec3(0, 0, height_to_c);
    EmitVertex();
    EndPrimitive();
}