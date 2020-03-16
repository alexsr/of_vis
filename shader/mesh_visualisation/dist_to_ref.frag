#version 450

#extension GL_ARB_bindless_texture : require

in float pass_dist;
in vec4 pass_pos;
in vec3 pass_normal;

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
};

layout(bindless_sampler) uniform sampler2D colormap;
uniform float min_dist;
uniform float max_dist;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 light_vec = normalize(vec3(light_pos - pass_pos));
    float cos_phi = max(dot(normalize(pass_normal), light_vec), 0.0);
    float mapped_dist = (pass_dist - min_dist) / (max_dist - min_dist);
    vec4 color = vec4(texture(colormap, vec2(clamp(mapped_dist, 0, 1), 0.5)).rgb, 1);
    out_color = color * ambient_color + color * cos_phi * light_color;
}
