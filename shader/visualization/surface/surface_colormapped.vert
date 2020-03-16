#version 450

#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in float scalar;

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

out vec4 pass_pos;
out vec3 pass_normal;
out float pass_scalar;

void main() {
    pass_pos = view * position;
    mat4 inv_view = inverse(view);
    vec3 check_normal_dir = position.xyz - inv_view[3].xyz;
    pass_normal = vec3(transpose(inv_view) * normal);
    pass_scalar = scalar;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((position - clipping[i].pos).xyz, normalize(clipping[i].normal.xyz));
    }
    gl_Position = proj * pass_pos;
}
