#version 450

uniform mat4 inverse_vp;
uniform vec4 origin;

out vec2 pass_uv;
out vec4 ray_dir;

void main() {
    pass_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vec4 tp = inverse_vp * vec4(pass_uv * 2.0f + -1.0f, 0.0f, 1.0f);
    ray_dir = tp - origin;
    gl_Position = vec4(pass_uv * 2.0f + -1.0f, 0.0f, 1.0f);
}
