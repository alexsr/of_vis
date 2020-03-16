#version 450

in vec4 pass_pos;
in vec3 pass_normal;

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
    float shininess;
};

out vec4 frag_color;

void main() {
    vec3 light_vec = normalize((light_pos - pass_pos).xyz);
    float cos_phi = max(dot(normalize(pass_normal), light_vec), 0.0);
    vec3 eye = normalize(vec3(-pass_pos));
    vec3 reflection = normalize(reflect(-light_vec, pass_normal));
    float cos_psi_n = pow(max(dot(reflection, eye), 0.0), shininess);
    frag_color = vec4(0.2, 0.2, 0.2, 1.0);
    frag_color = frag_color * ambient_color + frag_color * cos_phi + frag_color * cos_psi_n;
    frag_color.a = 1.0f;
}
