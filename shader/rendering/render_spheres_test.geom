#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 view;
uniform mat4 proj;

flat in vec4 pass_pos_world_g[];
flat in vec4 pass_pos_g[];
flat in float pass_radius_g[];
flat in float pass_scalar_g[];

flat out vec4 pass_world_pos;
flat out vec4 pass_pos;
out vec2 pass_uv;
flat out float pass_radius;
flat out float pass_scalar;

void main() {
    pass_uv = vec2(0,1);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(-1.0f, 1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(0,0);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(-1.0f, -1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(1,1);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(1.0f, 1.0f, 0.0f, 0.0f));
    EmitVertex();

    pass_uv = vec2(1,0);
    pass_world_pos = pass_pos_world_g[0];
    pass_pos = pass_pos_g[0];
    pass_radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (pass_pos_g[0] + pass_radius_g[0] * vec4(1.0f, -1.0f, 0.0f, 0.0f));
    EmitVertex();

    EndPrimitive();
}
