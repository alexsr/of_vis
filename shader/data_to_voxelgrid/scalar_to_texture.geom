#version 450 core

layout(points) in;
layout(invocations = 32) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 view;
uniform mat4 proj;
uniform int max_layers;

layout (std430, binding = VOLUME_GRID_BUFFER) buffer volume_grid {
    vec4 bb_min;
    vec4 bb_max;
    float cell_size;
    float cell_diag;
};

flat in vec4 pass_pos_g[];
flat in float pass_radius_g[];
flat in float pass_scalar_g[];

flat out float pass_scalar;

void main() {
    float radius = pass_radius_g[0];
    pass_scalar = pass_scalar_g[0];
    float step_size = 2.0 * radius / 32.0f;
    float curr_offset = gl_InvocationID * step_size;
    float curr_z = pass_pos_g[0].z - curr_offset;
    int layer = int((curr_z - bb_min.z) / cell_size);
    float result_z = bb_min.z + layer * cell_size;
    if (layer < 0) {
        return;
    }
    if (layer > max_layers) {
        return;
    }
    float h = curr_z - result_z;
    radius = sqrt(radius * 2.0f * h - h * h);
    pass_scalar = pass_scalar_g[0];
    vec4 export_pos = view * vec4(pass_pos_g[0].xy, 0.0f, 1.0f);
    export_pos.z = mix(-cell_size / 2.0f, -1.5f * cell_size, curr_offset / (2.0f * radius));
    gl_Position = proj * (export_pos + radius * vec4(-1.0f, 1.0f, 0.0f, 0.0f));
    gl_Layer = layer;
    EmitVertex();

    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (export_pos + radius * vec4(-1.0f, -1.0f, 0.0f, 0.0f));
    gl_Layer = layer;
    EmitVertex();

    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (export_pos + radius * vec4(1.0f, 1.0f, 0.0f, 0.0f));
    gl_Layer = layer;
    EmitVertex();

    pass_scalar = pass_scalar_g[0];
    gl_Position = proj * (export_pos + radius * vec4(1.0f, -1.0f, 0.0f, 0.0f));
    gl_Layer = layer;
    EmitVertex();
    EndPrimitive();
}
