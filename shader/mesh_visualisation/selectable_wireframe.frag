#version 450

in vec3 pass_normal;
in vec3 color;
noperspective in vec3 dist_to_edge;

uniform vec3 select_wireframe_color;
uniform float line_width;

out vec4 frag_color;

// https://catlikecoding.com/unity/tutorials/advanced-rendering/flat-and-wireframe-shading/

void main() {
    vec3 deltas = fwidth(dist_to_edge);
    vec3 thickness = deltas * line_width * gl_FragCoord.w;
    vec3 smoothing = deltas * log2(line_width + 1);
    vec3 dist = smoothstep(thickness, thickness + smoothing, dist_to_edge);
    float min_dist = min(dist.x, min(dist.y, dist.z));
    frag_color = vec4(mix(color, pass_normal, min_dist), 1.0f);

}
