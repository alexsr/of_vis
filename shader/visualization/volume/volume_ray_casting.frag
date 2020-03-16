#version 450

#extension GL_ARB_bindless_texture : require

in vec2 pass_uv;
in vec4 ray_dir;
flat in vec3 origin;

layout (std430, binding = MIN_MAX_BUFFER) buffer minmax_ssbo {
    float min_scalar;
    float max_scalar;
};

layout (std430, binding = VOLUME_GRID_BUFFER) buffer volume_grid {
    vec4 bb_min;
    vec4 bb_max;
    float cell_size;
    float cell_diag;
};

struct Clip_plane {
    vec4 pos;
    vec4 normal;
};

layout (std430, binding = CLIPPING_PLANE_BUFFER) buffer clipping_planes {
    Clip_plane clipping[];
};

uniform int clip_plane_count = 1;

layout(bindless_sampler) uniform sampler3D data;
layout(bindless_sampler) uniform sampler2D colormap;

out vec4 fragColor;

float max_value(vec3 vec) {
    return max(vec.x, max(vec.y, vec.z));
}

float min_value(vec3 vec) {
    return min(vec.x, min(vec.y, vec.z));
}

bool ray_aabb_intersect(vec3 r_o, vec3 r_dir, vec3 b_min, vec3 b_max, out float t_min, out float t_max) {
    vec3 inv_dir = 1.f / r_dir;
    vec3 t_b_min = (b_min - r_o) * inv_dir;
    vec3 t_b_max = (b_max - r_o) * inv_dir;
    t_min = max_value(min(t_b_min, t_b_max));
    t_max = min_value(max(t_b_min, t_b_max));
    return t_min < t_max;
}

bool ray_plane_intersection(vec3 r_o, vec3 r_dir, Clip_plane plane, out float t, out bool front_half_space) {
    float denom = dot(plane.normal.xyz, r_dir);
    vec3 ray_to_plane = plane.pos.xyz - r_o;
    float dot_normals = dot(ray_to_plane, plane.normal.xyz);
    front_half_space = denom < 0 ? false : true;
    if (denom == 0) {
        t = 0.0f;
        return false;
    }
    t = dot_normals / denom;
    return (t >= 0);
}

vec3 gen_3d_uv(vec3 p) {
    return (p - vec3(bb_min)) / vec3(bb_max - bb_min);
}

float map_between_zero_and_one(float s, float min_v, float max_v) {
    return (s - min_v) / (max_v - min_v);
}

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};


void main() {
    vec3 norm_dir = normalize(ray_dir.xyz);
    ivec3 volume_size = textureSize(data, 0);
    float t_min = 0.0f;
    float t_max = 0.0f;
    bool intersects = ray_aabb_intersect(origin, norm_dir, bb_min.xyz, bb_max.xyz, t_min, t_max);
    for (int i = 0; i < clip_plane_count; ++i) {
        float t_plane = t_max;
        bool front_half_space = false;
        bool plane_intersection = ray_plane_intersection(origin, norm_dir, clipping[i], t_plane, front_half_space);
        if (plane_intersection) {
            if (!front_half_space) {
                t_max = min(t_plane, t_max);
            }
            else {
                t_min = max(t_plane, t_min);
            }
        }
        else {
            if (!front_half_space) {
                t_min = t_max;
            }
        }
    }
    float t = t_min;
    int max_steps = int(1.735f * max_value(vec3(volume_size)));
    vec4 color = vec4(0);
    if (intersects) {
        int curr_step = 0;
        while (t < t_max && curr_step < max_steps && color.a < 0.95f) {
            vec3 pos = origin + t * norm_dir;
            vec2 curr_scalar = texture(data, gen_3d_uv(pos)).ra;
            if (curr_scalar.y == 1.0f) {
                float mapped_scalar = clamp(map_between_zero_and_one(curr_scalar.x, min_scalar, max_scalar), 0, 1);
                float curr_alpha = 1.0 - pow(1.0 - mapped_scalar, 1000.0f * cell_size);
                if (curr_alpha > 0.0f) {
                    vec4 curr_color = vec4(texture(colormap, vec2(mapped_scalar, 0.5)).rgb * curr_alpha, curr_alpha);
                    color += (1.0f - color.a) * curr_color;
                }
            }
            t += cell_size;
            ++curr_step;
        }
        if (color.a > 1.0f) {
            color.a = 1.0f;
        }
        color.rgb /= color.a;
        fragColor = color;
    }
    else {
        fragColor = vec4(0);
        discard;
    }
}
