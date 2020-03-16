#version 450

#extension GL_ARB_bindless_texture : require

in vec4 pass_pos;
in vec3 pass_normal;
in float pass_scalar;

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
    float shininess;
};

layout (std430, binding = MIN_MAX_BUFFER) buffer minmax_ssbo {
    float min_scalar;
    float max_scalar;
};

layout(bindless_sampler) uniform sampler2D colormap;

out vec4 frag_color;

void main() {
    vec3 normal = pass_normal;
    if (!gl_FrontFacing) {
        normal = -1.0f * normal;
    }
    vec3 light_vec = normalize(vec3(light_pos - pass_pos));
    float cos_phi = max(dot(normalize(normal), light_vec), 0.0);
    float mapped_scalar = clamp((pass_scalar - min_scalar) / (max_scalar - min_scalar), 0, 1);
    frag_color = vec4(texture(colormap, vec2(mapped_scalar, 0.5)).rgb, 1);
    frag_color = frag_color * ambient_color + frag_color * cos_phi * light_color;
}
