#version 450

layout (location = 0) in vec4 position;

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

void main() {
    vec3 origin = inverse(view)[3].xyz;
    for (int i = 0; i < clip_plane_count; ++i) {
        if (dot(clipping[i].normal.xyz, normalize(origin - clipping[i].pos.xyz)) >= 0.0f) {
            gl_ClipDistance[i] = 1.0f;
        }
        else {
        gl_ClipDistance[i] = dot((position - clipping[i].pos).xyz, clipping[i].normal.xyz);
        }
        //gl_ClipDistance[i] = dot((position - clipping[i].pos).xyz, clipping[i].normal.xyz);
    }
    gl_Position = proj * view * position;
}
