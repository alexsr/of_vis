#version 450

in vec2 pass_uv;
flat in vec4 pass_pos;
flat in float pass_radius;
flat in vec4 pass_color;

uniform mat4 view;
uniform mat4 proj;

out vec4 fragColor;
out float gl_FragDepth;

void main() {
    vec3 N;
    N.xy = pass_uv * 2.0 - vec2(1.0);
    float mag = 1 - dot(N.xy, N.xy);
    if (mag <= 0.0f) {
        discard;
    }
    N.z = sqrt(mag);
    vec4 actual_pos = proj * (pass_pos - vec4(0, 0, pass_radius, 0) + vec4(pass_radius * N, 0));
    actual_pos = actual_pos / actual_pos.w;
    gl_FragDepth = 0.5 + 0.5 * actual_pos.z;
    fragColor = pass_color;
    fragColor.w = 1;
}
