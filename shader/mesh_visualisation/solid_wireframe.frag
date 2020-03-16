#version 450

in vec3 pass_color;
noperspective in vec3 dist_to_edge;

out vec4 frag_color;

uniform vec3 wireframe_color;
uniform float line_width;

void main() {
    vec3 deltas = fwidth(dist_to_edge);
    vec3 thickness = deltas * line_width * gl_FragCoord.w;
    vec3 smoothing = deltas * log2(line_width + 1);
    vec3 dist = smoothstep(thickness, thickness + smoothing, dist_to_edge);
    float min_dist = min(dist.x, min(dist.y, dist.z));
    frag_color = vec4(mix(wireframe_color, pass_color, min_dist), 1.0f);
}
