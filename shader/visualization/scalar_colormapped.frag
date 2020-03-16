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
    float scalar = texture(data, vec3(k, pass_uv)).r;
    scalar = clamp((scalar - min_scalar) / (max_scalar - min_scalar), 0, 1);
    fragColor = vec4(texture(colormap, vec2(scalar, 0.5)).rgb, 1);
}
