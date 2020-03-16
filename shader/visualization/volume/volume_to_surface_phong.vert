#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;

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

out vec4 pass_pos;
out vec4 pass_normal;
out vec4 pass_phong_normal;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

void main() {
    pass_pos = position;
    pass_normal = normal;
    pass_phong_normal = vec4(transpose(inverse(mat3(view))) * normal.xyz, 0.0f);
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((position - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * view * position;
}
