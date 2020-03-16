#version 450

#extension GL_ARB_bindless_texture : require

flat in vec4 pass_world_pos;
flat in vec4 pass_pos;
in vec2 pass_uv;
flat in float pass_radius;
flat in float pass_scalar;

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

layout (std430, binding = MIN_MAX_BUFFER) buffer minmax_ssbo {
    float min_scalar;
    float max_scalar;
};

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
    float shininess;
};

layout(bindless_sampler) uniform sampler2D colormap;

out vec4 frag_color;
layout (depth_greater) out float gl_FragDepth;

void main() {
    vec3 N;
    N.xy = pass_uv * 2.0 - vec2(1.0);
    float mag = 1.0 - dot(N.xy, N.xy);
    if (mag <= 0.0) {
        discard;
    }
    N.z = sqrt(mag);
    vec4 actual_pos = proj * (pass_pos - vec4(0, 0, pass_radius, 0) + vec4(pass_radius * N, 0));
    actual_pos = actual_pos / actual_pos.w;
    gl_FragDepth = 0.5 + 0.5 * actual_pos.z;
    float scalar = clamp((pass_scalar - min_scalar) / (max_scalar - min_scalar), 0, 1);
    frag_color = vec4(texture(colormap, vec2(scalar, 0.5)).rgb, 1);
    
    vec3 light_vec = normalize(vec3(light_pos - pass_pos));
    float cos_phi = max(dot(normalize(N), light_vec), 0.0);
    frag_color = frag_color * ambient_color + frag_color * cos_phi * light_color;
}
