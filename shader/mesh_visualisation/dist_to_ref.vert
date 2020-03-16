#version 460

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

uniform mat4 view;
uniform mat4 proj;

layout (std430, binding = DIST_BUFFER) buffer distance_ssbo {
    float dist[];
};

out float pass_dist;
out vec4 pass_pos;
out vec3 pass_normal;

void main() {
    pass_dist = dist[gl_VertexID];
    pass_pos = view * position;
    pass_normal = vec3(transpose(inverse(view)) * normal);
    gl_Position = proj * pass_pos;
}
