#version 450

#extension GL_ARB_bindless_texture : require

in vec2 pass_uv;

out vec4 fragColor;

layout(bindless_sampler) uniform sampler2D tex;

void main() {
    fragColor = texture(tex, pass_uv);
}
