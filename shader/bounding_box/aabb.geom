#version 450

layout (lines) in;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (triangle_strip, max_vertices = 14) out;
void main() {
    vec4 min_vert = gl_in[0].gl_Position;
    vec4 max_vert = gl_in[1].gl_Position;
    gl_Position = proj * view * vec4(min_vert.x, max_vert.y, max_vert.z, 1.0); // 4
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, max_vert.y, max_vert.z, 1.0); // 3
    EmitVertex();
    gl_Position = proj * view  * vec4(min_vert.x, min_vert.y, max_vert.z, 1.0); // 7
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, min_vert.y, max_vert.z, 1.0); // 8
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, min_vert.y, min_vert.z, 1.0); // 5
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, max_vert.y, max_vert.z, 1.0); // 3
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, max_vert.y, min_vert.z, 1.0); // 1
    EmitVertex();
    gl_Position = proj * view  *  vec4(min_vert.x, max_vert.y, max_vert.z, 1.0); // 4
    EmitVertex();
    gl_Position = proj * view  *  vec4(min_vert.x, max_vert.y, min_vert.z, 1.0); // 2
    EmitVertex();
    gl_Position = proj * view  * vec4(min_vert.x, min_vert.y, max_vert.z, 1.0); // 7
    EmitVertex();
    gl_Position = proj * view  * vec4(min_vert.x, min_vert.y, min_vert.z, 1.0); // 6
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, min_vert.y, min_vert.z, 1.0); // 5
    EmitVertex();
    gl_Position = proj * view  * vec4(min_vert.x, max_vert.y, min_vert.z, 1.0); // 2
    EmitVertex();
    gl_Position = proj * view  * vec4(max_vert.x, max_vert.y, min_vert.z, 1.0); // 1
    EmitVertex();
    EndPrimitive();
}
