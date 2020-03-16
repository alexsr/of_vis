#version 450

#extension GL_ARB_bindless_texture : require

in vec2 pass_uv;

uniform float min_scalar;
uniform float max_scalar;

out vec4 fragColor;

layout(bindless_sampler) uniform sampler2D tex;

void main() {
    float scalar = texture(tex, pass_uv).r;
    if (scalar < min_scalar) {
        discard;
    }
    scalar = (scalar - min_scalar) / (max_scalar - min_scalar);
    fragColor = vec4(scalar, 0, 0, 1);
}
