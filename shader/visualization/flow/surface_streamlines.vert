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

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

out vec4 pass_pos;
out vec3 pass_normal;

void main() {
    pass_pos = view * vec4(position.xyz, 1.0f);
    pass_normal = (transpose(inverse(view)) * normal).xyz;
    for (int i = 0; i < clip_plane_count; ++i) {
        gl_ClipDistance[i] = dot((position - clipping[i].pos).xyz, normalize(clipping[i].normal.xyz));
    }
    gl_Position = proj * pass_pos;
}
