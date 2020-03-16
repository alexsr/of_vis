#version 430 core

layout(location = 0) in vec4 pos;
layout(location = 1) in float radius;
layout(location = 2) in float scalar;

uniform mat4 view;
uniform mat4 proj;

flat out vec4 pass_pos_world_g;
flat out vec4 pass_pos_g;
flat out float pass_radius_g;
flat out float pass_scalar_g;

void main(){
    pass_pos_world_g = pos;
    pass_pos_g = view * pos;
    pass_radius_g = radius;
    pass_scalar_g = scalar;
    gl_Position = proj * pass_pos_g;
}
