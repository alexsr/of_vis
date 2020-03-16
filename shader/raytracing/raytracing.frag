#version 450

#extension GL_ARB_bindless_texture : require

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

in vec2 pass_uv;
in vec4 ray_dir;

uniform vec3 box_min;
uniform vec3 box_max;
uniform vec4 origin;

out vec4 fragColor;

layout(bindless_sampler) uniform sampler2D tex;

void main() {
    vec3 norm_dir = vec3(normalize(ray_dir));
    vec3 ray_origin = vec3(origin);
    float t_min, t_max;
    fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    if (ray_aabb_intersect(ray_origin, norm_dir, box_min, box_max, t_min, t_max)) {
        fragColor = vec4(texture(tex, pass_uv));
    }
    else {
        discard;
    }
}
