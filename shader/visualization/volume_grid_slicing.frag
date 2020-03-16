#version 450

#extension GL_ARB_bindless_texture : require

in vec2 pass_uv;

uniform float min_scalar;
uniform float max_scalar;
uniform float k;

out vec4 fragColor;

layout(bindless_sampler) uniform sampler3D data;
layout(bindless_sampler) uniform sampler2D colormap;

void main() {
    float scalar = texture(data, vec3(pass_uv, k)).r;
    scalar = (scalar - min_scalar) / (max_scalar - min_scalar);
    if (scalar < 0.0f || scalar > max_scalar) {
        discard;
    }
    fragColor = vec4(texture(colormap, vec2(clamp(scalar, 0.0f, 1.0f), 0.5)).rgb, 1);
}
