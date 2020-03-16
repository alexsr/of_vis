#version 430 core

layout(location = 0) in vec4 pos;
layout(location = 1) in float radius;
layout(location = 2) in float scalar;

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

layout (std430, binding = VOLUME_GRID_BUFFER) buffer volume_grid {
    vec4 bb_min;
    vec4 bb_max;
    float cell_size;
    float cell_diag;
};

uniform int clip_plane_count = 0;

flat out vec4 pass_pos_world_g;
flat out vec4 pass_pos_g;
flat out float pass_radius_g;
flat out float pass_scalar_g;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

void main(){
    pass_pos_world_g = pos;
    pass_pos_g = view * pass_pos_world_g;
    pass_radius_g = radius;
    pass_scalar_g = scalar;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pos - clipping[i].pos).xyz, normalize(clipping[i].normal.xyz));
    }
    gl_Position = proj * pass_pos_g;
}
