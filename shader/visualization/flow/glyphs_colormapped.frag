#version 460

#extension GL_ARB_bindless_texture : require

in vec4 pass_world_pos;
in vec4 pass_pos;
in vec4 tangent;
in vec2 pass_uv;

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
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

float dot2(vec3 v) {
	return dot(v, v);
}
float dot2(vec2 v) {
	return dot(v, v);
}

float udQuad(vec3 p, vec3 a, vec3 b, vec3 c, vec3 d) {
    vec3 ba = b - a; vec3 pa = p - a;
    vec3 cb = c - b; vec3 pb = p - b;
    vec3 dc = d - c; vec3 pc = p - c;
    vec3 ad = a - d; vec3 pd = p - d;
    vec3 nor = cross(ba, ad);

    return (sign(dot(cross(ba, nor), pa)) +
     sign(dot(cross(cb, nor) ,pb)) +
     sign(dot(cross(dc, nor), pc)) +
     sign(dot(cross(ad, nor), pd)) < 3.0 ? 1.0 : -1.0)
     * sqrt((sign(dot(cross(ba, nor), pa)) +
     sign(dot(cross(cb, nor), pb)) +
     sign(dot(cross(dc, nor), pc)) +
     sign(dot(cross(ad, nor), pd))<3.0)
     ? min(min(min(dot2(ba * clamp(dot(ba,pa) / dot2(ba), 0.0, 1.0) - pa),
     dot2(cb * clamp(dot(cb, pb) / dot2(cb), 0.0, 1.0) - pb)),
     dot2(dc * clamp(dot(dc, pc) / dot2(dc), 0.0, 1.0) - pc)),
     dot2(ad * clamp(dot(ad, pd) / dot2(ad), 0.0, 1.0) - pd))
     : dot(nor, pa) * dot(nor, pa) / dot2(nor));
}

float arrow(vec2 position, float height, float len, float tip_len, float tip_height) {
	float strip_pos_x = -len;
	vec2 triangle_pos = vec2(len, 0) - 1.0 * vec2(tip_len, 0);
    vec3 quad_a = vec3(strip_pos_x, height, 0.0);
    vec3 quad_b = vec3(strip_pos_x, -height, 0.0);
    vec3 quad_c = vec3(triangle_pos.x, -height, 0.0);
    vec3 quad_d = vec3(triangle_pos.x, height, 0.0);

	vec2 triangle_tip = triangle_pos + vec2(tip_len, 0.0);
    
    vec2 a = vec2(triangle_pos.x, tip_height);
    vec2 b = vec2(triangle_pos.x, -tip_height);
    vec2 c = vec2(triangle_tip);

    vec3 p = vec3(position, 0.0);
    vec2 ba = b - a;
    vec2 cb = c - b;
    vec2 ac = a - c;

    vec2 pa = p.xy - a;
    vec2 pb = p.xy - b;
    vec2 pc = p.xy - c;

    float edge_ab = dot(vec2(-ba.y, ba.x), pa);
    float edge_bc = dot(vec2(-cb.y, cb.x), pb);
    float edge_ca = dot(vec2(-ac.y, ac.x), pc);

    float v = sign(edge_ab) + sign(edge_bc) + sign(edge_ca);

    float triangle = ((v < 2.0) ? 1.0 : -1.0)
    * sqrt(min(min(dot2(ba * clamp(dot(ba, pa) / dot2(ba), 0.0, 1.0) - pa),
    dot2(cb * clamp(dot(cb, pb) / dot2(cb), 0.0, 1.0) - pb)),
    dot2(ac * clamp(dot(ac, pc) / dot2(ac), 0.0, 1.0) - pc)));
	return min(udQuad(p, quad_a, quad_b, quad_c, quad_d), triangle);
}

out vec4 frag_color;

void main() {
    vec3 uv = gen_3d_uv(pass_world_pos.xyz);
    float scalar = clamp((texture(data, uv).r - min_scalar) / (max_scalar - min_scalar), 0, 1);
    vec4 scalar_color = vec4(texture(colormap, vec2(scalar, 0.5)).rgb, 1);
    vec3 light_vec = normalize(light_pos.xyz - pass_pos.xyz);
    vec3 norm_tangent = normalize(tangent.xyz);
    float dot_l_t = dot(norm_tangent, light_vec);
    float cos_phi = sqrt(1.0f - dot_l_t * dot_l_t);
    frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    float arrow_dist = arrow(2.0 * pass_uv - 1.0, 0.2, 0.6, 0.5, 0.75);
    if (arrow_dist > 0.1) {
        discard;
    }
    scalar_color = mix(vec4(0), scalar_color, abs(1.0 - arrow_dist) * abs(1.0 - arrow_dist));
    if (arrow_dist > 0.0) {
        scalar_color = vec4(0.0);
    }
    frag_color = 0.5 * scalar_color + 0.5 * scalar_color * cos_phi * light_color;// + light_color * cos_phi_n * 30;
    frag_color.a = 0.9;
}
