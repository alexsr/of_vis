#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;

uniform mat4 view;
uniform mat4 proj;
uniform float k;
uniform vec4 bb_min;
uniform vec4 bb_max;

out vec4 pass_pos;
out vec4 pass_normal;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_ClipDistance[1];
};

void main() {
    pass_pos = position;
    vec4 plane_pos = vec4(0, 0, (bb_max - k * (bb_max - bb_min)).z, 1.0f);
    pass_normal = normal;
    gl_Position = proj * view * position;
    gl_ClipDistance[0] = dot(position - plane_pos, vec4(0, 0, -1, 0));
}
