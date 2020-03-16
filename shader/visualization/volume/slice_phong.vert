#version 450

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

layout (std430, binding = VOLUME_GRID_BUFFER) buffer volume_grid {
    vec4 bb_min;
    vec4 bb_max;
    float cell_size;
    float cell_diag;
};

out vec4 pass_pos;
out vec4 pass_normal;
out vec4 pass_phong_normal;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[];
};

void main() {
    float size = 3.0f * length((bb_max - bb_min).xyz);
    vec4 plane_point_vertex = vec4((vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2) - 1.0f), 0.0f, 1.0f);
    vec4 plane_pos = clipping[gl_InstanceID].pos;
    plane_pos.xyz = clamp(plane_pos.xyz, bb_min.xyz * 3.0f, bb_max.xyz * 3.0f);
    vec4 plane_normal = clipping[gl_InstanceID].normal;
    vec3 abs_normal = abs(plane_normal.xyz);
    vec3 dir = -normalize(plane_normal.xyz);
    vec3 up = vec3(0, 1, 0);
    if (abs_normal.y > max(abs_normal.x, abs_normal.z)) {
        up = vec3(1, 0, 0);
    }
    vec3 s = normalize(cross(dir, up));
	vec3 u = normalize(cross(s, dir));
    mat3 plane_view = mat3(1.0f);
    plane_view[0][0] = s.x;
    plane_view[0][1] = s.y;
    plane_view[0][2] = s.z;
    plane_view[1][0] = u.x;
    plane_view[1][1] = u.y;
    plane_view[1][2] = u.z;
    plane_view[2][0] = dir.x;
    plane_view[2][1] = dir.y;
    plane_view[2][2] = dir.z;
    plane_point_vertex = vec4(plane_pos.xyz + normalize(plane_view * plane_point_vertex.xyz) * size, 1.0f);
    pass_pos = plane_point_vertex;
    pass_phong_normal = vec4(-transpose(inverse(mat3(view))) * plane_normal.xyz, 0.0f);
    pass_normal = plane_normal;
    for (int i = 0; i < clip_plane_count; ++i) {
        if (i == gl_InstanceID) {
            gl_ClipDistance[i] = 1.0f;
        }
        else {
            gl_ClipDistance[i] = dot((plane_point_vertex - clipping[i].pos).xyz, clipping[i].normal.xyz);
        }
    }
    gl_Position = proj * view * plane_point_vertex;
}
