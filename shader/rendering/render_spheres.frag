#version 450

in vec2 pass_uv;
flat in vec4 pass_pos;
flat in float pass_radius;

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
    fragColor = vec4(pass_scalar, 0, 0, distance(actual_pos, pass_pos));
    gl_FragDepth = 0.5 + 0.5 * actual_pos.z;
    vec3 lightPos = (view * vec4(-1.7,1.7,1.7,1.0)).xyz;
    vec3 lightVec = normalize(lightPos - pass_pos.xyz);
    vec3 eye = normalize(-pass_pos.xyz);
    vec3 r = normalize(reflect(-lightVec, N));
    float d = max(dot(lightVec, N),0);
    fragColor = 0.9 * d * fragColor + 0.1 * fragColor;
    //float srg = 0.2 * length(passVel.xyz);
    //fragmentColor += vec4(srg);
    fragColor.w = 1;
}
