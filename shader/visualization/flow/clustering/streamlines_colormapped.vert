#version 460 core

layout (std430, binding = STREAMLINES_SSBO_BINDING) buffer lines_ssbo {
    vec4 line[];
};

layout (std430, binding = CLUSTER_IDS) buffer cluster_ssbo {
    int cluster[];
};

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
uniform float radius;

uniform int step_count;

out vec4 pass_pos_world_g;
flat out float pass_radius_g;
flat out int pass_cluster_g;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

void main(){
    pass_pos_world_g = vec4(line[gl_InstanceID * step_count + gl_VertexID].xyz, 1.0f);
	pass_cluster_g = cluster[gl_InstanceID];
    pass_radius_g = radius;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((pass_pos_world_g - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * view * pass_pos_world_g;
}
