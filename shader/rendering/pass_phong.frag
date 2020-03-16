#version 460

in vec4 pass_pos;
in vec3 pass_normal;
in vec4 pass_color;

uniform mat4 view;

layout (std430, binding = PHONG_BINDING) buffer phong_buffer {
    vec4 color;
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    float shininess;
};

out vec4 frag_color;

void main() {
    vec3 light_vec = normalize(vec3(light_pos - pass_pos));
    float cos_phi = max(dot(pass_normal, light_vec), 0.0);

    vec3 eye = normalize(vec3(-pass_pos));
    vec3 reflection = normalize(reflect(-light_vec, pass_normal));
    float cos_psi_n = pow(max(dot(reflection, eye), 0.0), shininess);

    frag_color = pass_color * ambient_color + pass_color * cos_phi * light_color + pass_color * cos_psi_n * light_color;
}
