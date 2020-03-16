#version 460

#extension GL_ARB_bindless_texture : require

in vec4 pass_world_pos;
in vec4 pass_pos;
in vec4 tangent;
in vec2 pass_uv;
in flat int pass_cluster;

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

layout (std430, binding = CLUSTER_COLORS) buffer cluster_cols_ssbo {
    vec4 cluster_cols[];
};

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

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
    float shininess;
};

layout(bindless_sampler) uniform sampler3D data;
layout(bindless_sampler) uniform sampler2D colormap;

vec3 gen_3d_uv(vec3 p) {
    return (p - bb_min.xyz) / (bb_max - bb_min).xyz;
}

out vec4 frag_color;

void main() {
    vec4 scalar_color = cluster_cols[pass_cluster];
    vec3 light_vec = normalize(light_pos.xyz - pass_pos.xyz);
    float scale = 2.0f * abs(pass_uv.y - 0.5);
    scalar_color = mix(scalar_color, vec4(0), scale * scale);
    vec3 norm_tangent = normalize(tangent.xyz);
    float dot_l_t = dot(norm_tangent, light_vec);
    float dot_v_t = dot(norm_tangent, -normalize(pass_pos.xyz));
    float cos_phi = sqrt(1.0f - dot_l_t * dot_l_t);
    float cos_phi_n = pow(max(dot_l_t * dot_v_t - cos_phi * sqrt(1.0f - dot_v_t * dot_v_t), 0.0f), shininess);
    frag_color = scalar_color * cos_phi * light_color + light_color * scalar_color * cos_phi_n * 0.6;
    frag_color.a = 0.7;
}
