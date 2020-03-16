#version 450

#extension GL_ARB_bindless_texture : require

in vec4 pass_pos;
in vec4 pass_normal;
in vec4 pass_phong_normal;

layout (std430, binding = MIN_MAX_BUFFER) buffer minmax_ssbo {
    float min_scalar;
    float max_scalar;
};

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

layout (std430, binding = VOLUME_GRID_BUFFER) buffer volume_grid {
    vec4 bb_min;
    vec4 bb_max;
    float cell_size;
    float cell_diag;
};

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
    float shininess;
};

vec3 gen_3d_uv(vec3 p) {
    return (p - vec3(bb_min)) / vec3(bb_max - bb_min);
}

out vec4 frag_color;

layout(bindless_sampler) uniform sampler3D data;
layout(bindless_sampler) uniform sampler2D colormap;

void main() {
    vec3 normal = normalize(pass_normal.xyz);
    vec3 pos = pass_pos.xyz + normal * cell_diag;
    vec3 uv = gen_3d_uv(pos);
    vec2 scalar = texture(data, uv).ra;
    float normal_step = 0.0f;
    int step_count = 0;
    while (scalar.y < 1.0f && step_count < 50) {
        pos -= normal * cell_diag * 0.1f;
        uv = gen_3d_uv(pos);
        scalar = texture(data, uv).ra;
        step_count++;
    }
    float mapped_scalar = (scalar.x - min_scalar) / (max_scalar - min_scalar);
    frag_color = vec4(texture(colormap, vec2(clamp(mapped_scalar, 0, 1), 0.5)).rgb, 1);
    vec3 light_vec = normalize((light_pos - pass_pos).xyz);
    float cos_phi = max(dot(normalize(pass_phong_normal.xyz), light_vec), 0.0);
    frag_color = frag_color * cos_phi * light_color;
}
