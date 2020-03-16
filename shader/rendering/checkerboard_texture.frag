#version 450

#extension GL_ARB_bindless_texture : require

in vec2 pass_uv;

out vec4 fragColor;

uniform float type = 3.0f;
uniform float scale = 5.0f;
uniform float aspect_ratio = 1.0f;
uniform vec2 mouse_uv;
uniform float mouse_radius = 0.1f;
uniform bool pattern = true;

layout(bindless_sampler) uniform sampler2D tex;

float crosses(vec2 p, vec2 dpdx, vec2 dpdy, float N) {
    vec2 w = max(abs(dpdx), abs(dpdy));
    vec2 a = p + 0.5 * w;
    vec2 b = p - 0.5 * w;
    vec2 i = (floor(a) + min(fract(a) * N, 1.0) -
              floor(b) - min(fract(b) * N, 1.0)) / (N * w);
    return 1.0 - i.x - i.y + 2.0 * i.x * i.y;
}

void main() {
    vec2 scaled_uv = vec2(pass_uv.x * aspect_ratio, pass_uv.y);
    vec2 scaled_mouse = vec2(mouse_uv.x * aspect_ratio, mouse_uv.y);
    float dist = distance(scaled_uv, scaled_mouse);
    if (dist > mouse_radius) {
        discard;
    }
    float offset = 0.01f;
    if (dist < mouse_radius - offset && pattern && crosses(scaled_mouse * scale - scaled_uv * scale, dFdx(pass_uv), dFdy(pass_uv), type) < 0.5) {
        discard;
    }
    fragColor.rgb = mix(vec3(0, 0, 0), texture(tex, pass_uv).rgb, clamp(((mouse_radius - offset / 4.0f) - dist) / (offset / 4.0f), 0, 1));
    fragColor.a = 1.0f;
    fragColor.a = mix(fragColor.a, 0.0f, clamp((dist - (mouse_radius - offset / 4.0f)) / (offset / 4.0f), 0, 1));
}
