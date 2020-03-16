#version 430 core

layout(location = 0) in vec4 pos;
layout(location = 1) in float radius;
layout(location = 2) in float scalar;

flat out vec4 pass_pos_g;
flat out float pass_radius_g;
flat out float pass_scalar_g;

void main() {
    pass_radius_g = 2.0f * radius;
    pass_pos_g = vec4(pos.xyz, 1.0f);
    pass_scalar_g = scalar;
}
