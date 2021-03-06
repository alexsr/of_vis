#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

uniform vec4 seed_sphere_pos;
uniform float seed_radius;

out vec4 pass_pos;
out vec3 pass_normal;

void main() {
    pass_pos = view * vec4((seed_radius * position + seed_sphere_pos).xyz, 1.0f);
    pass_normal = (transpose(inverse(view)) * normal).xyz;
    gl_Position = proj * pass_pos;
}
