#version 450

#extension GL_ARB_bindless_texture : require

noperspective in vec4 pass_pos;
noperspective in vec4 pass_normal;

uniform float min_scalar;
uniform float max_scalar;
uniform vec4 bb_min;
uniform vec4 bb_max;
uniform float cell_diag;
uniform float cell_size;

vec3 gen_3d_uv(vec3 p) {
    return (p - vec3(bb_min)) / vec3(bb_max - bb_min);
}

out vec4 fragColor;

layout(bindless_sampler) uniform sampler3D data;
layout(bindless_sampler) uniform sampler2D colormap;

void main() {
    vec3 uv = gen_3d_uv(vec3(pass_pos));
    float scalar = texture(data, uv).r;
    float normal_step = 0.0f;
    int step_count = 0;
    while ((step_count < 1 || scalar < min_scalar) && step_count < 30) {
        uv = gen_3d_uv(vec3(pass_pos) - normalize(vec3(pass_normal)) * cell_diag * 1000.0f * normal_step);
        scalar = texture(data, uv).r;
        normal_step += cell_size * 1000.0f;
        step_count++;
    }
    scalar = (scalar - min_scalar) / (max_scalar - min_scalar);
    fragColor = vec4(texture(colormap, vec2(clamp(scalar, 0, 1), 0.5)).rgb, 1);
}
