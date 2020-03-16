#version 450

in vec4 pass_pos;
in vec3 pass_normal;

layout (std430, binding = DIFFUSE_LIGHTING) buffer diffuse_buffer {
    vec4 ambient_color;
    vec4 light_pos;
    vec4 light_color;
    vec4 object_color;
};

out vec4 frag_color;

void main() {
    vec3 light_vec = normalize(vec3(light_pos - pass_pos));
    float cos_phi = max(dot(normalize(pass_normal), light_vec), 0.0);
    frag_color = vec4(0.3f, 0.3f, 0.8f, 0.0f) * cos_phi * light_color;
    frag_color.a = 0.5f;
}
