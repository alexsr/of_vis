#version 460 core

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 pass_pos_world_g[];
flat in float pass_radius_g[];

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

struct Clip_plane {
    vec4 pos;
    vec4 normal;
};

layout (std430, binding = CLIPPING_PLANE_BUFFER) buffer clipping_planes {
    Clip_plane clipping[];
};

uniform int clip_plane_count = 0;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

out vec4 pass_world_pos;
out vec4 pass_pos;
out vec4 tangent;
out vec2 pass_uv;

void main() {
    vec4 line_p0 = view * pass_pos_world_g[0];
    vec4 line_p1 = view * pass_pos_world_g[1];
    vec4 line_p2 = view * pass_pos_world_g[2];
    vec4 line_p3 = view * pass_pos_world_g[3];
    
    vec4 temp_tangent_p1 = vec4(normalize((line_p2 - line_p0).xyz), 0.0f);
    vec4 temp_tangent_p2 = vec4(normalize((line_p3 - line_p1).xyz), 0.0f);
    vec4 sideways_vector_p1 = vec4(normalize(cross(normalize(line_p1.xyz), temp_tangent_p1.xyz)), 0.0f);
    vec4 sideways_vector_p2 = vec4(normalize(cross(normalize(line_p2.xyz), temp_tangent_p2.xyz)), 0.0f);

    tangent = temp_tangent_p1;
    pass_uv = vec2(0, 0);
    pass_world_pos = pass_pos_world_g[1];
    pass_pos = line_p1 - pass_radius_g[1] * sideways_vector_p1;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * pass_pos;
    EmitVertex();
    
    tangent = temp_tangent_p1;
    pass_uv = vec2(0, 1);
    pass_world_pos = pass_pos_world_g[1];
    pass_pos = line_p1 + pass_radius_g[1] * sideways_vector_p1;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * pass_pos;
    EmitVertex();
    
    tangent = temp_tangent_p2;
    pass_uv = vec2(1, 0);
    pass_world_pos = pass_pos_world_g[2];
    pass_pos = line_p2 - pass_radius_g[1] * sideways_vector_p2;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * pass_pos;
    EmitVertex();
    
    tangent = temp_tangent_p2;
    pass_uv = vec2(1, 1);
    pass_world_pos = pass_pos_world_g[2];
    pass_pos = line_p2 + pass_radius_g[1] * sideways_vector_p2;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * pass_pos;
    EmitVertex();
}
