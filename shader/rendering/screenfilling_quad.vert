#version 450

out vec2 pass_uv;

void main() {
    pass_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(pass_uv * 2.0f + -1.0f, 0.0f, 1.0f);
}
