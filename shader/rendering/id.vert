#version 450

layout (location = 0) in vec4 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

flat out int pass_id;

void main() {
    pass_id = gl_VertexID;
    gl_Position = proj * view * model * position;
}