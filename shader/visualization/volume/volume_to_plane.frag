#version 450

#extension GL_ARB_bindless_texture : require

in vec4 pass_pos;
in vec4 pass_normal;

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

vec3 gen_3d_uv(vec3 p) {
    return (p - vec3(bb_min)) / vec3(bb_max - bb_min);
}

out vec4 fragColor;

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
    fragColor = vec4(texture(colormap, vec2(clamp(mapped_scalar, 0, 1), 0.5)).rgb, 1);
}
