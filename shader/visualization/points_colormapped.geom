#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

flat in vec4 pass_pos_world_g[];
flat in vec4 pass_pos_g[];
flat in float pass_radius_g[];
flat in float pass_scalar_g[];

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

flat out vec4 pass_world_pos;
flat out vec4 pass_pos;
out vec2 pass_uv;
flat out float pass_radius;
flat out float pass_scalar;

void main() {
    pass_uv = vec2(0,1);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(-1.0f, 1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(0,0);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(-1.0f, -1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(1,1);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(1.0f, 1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(1,0);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_world_pos - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(1.0f, -1.0f, 0.0f, 0.0f));
    EmitVertex();

    EndPrimitive();
}
