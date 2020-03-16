#version 450

layout (std430, binding = CAM_BUFFER_BINDING) buffer cam_buffer {
    mat4 view;
    mat4 proj;
};

out vec2 pass_uv;
out vec4 ray_dir;
flat out vec3 origin;

void main() {
    pass_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    mat4 inv_view = inverse(view);
    vec4 tp = inv_view * inverse(proj) * vec4(pass_uv * 2.0f - 1.0f, 0.0f, 1.0f);
    tp /= tp.w;
    origin = inv_view[3].xyz;
    ray_dir = vec4(tp.xyz - origin, 0.0f);
    gl_Position = vec4(pass_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
