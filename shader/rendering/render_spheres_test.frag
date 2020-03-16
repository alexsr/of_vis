#version 450

#extension GL_ARB_bindless_texture : require

flat in vec4 pass_world_pos;
flat in vec4 pass_pos;
in vec2 pass_uv;
flat in float pass_radius;
flat in float pass_scalar;

uniform mat4 view;
uniform mat4 proj;
uniform float near;

out vec4 fragColor;
layout (depth_greater) out float gl_FragDepth;

void main() {
    vec3 N;
    N.xy = pass_uv * 2.0 - vec2(1.0);
    float mag = 1.0 - dot(N.xy, N.xy);
    if (mag <= 0.0) {
        discard;
    }
    N.z = sqrt(mag);
    vec4 actual_pos = proj * (pass_pos - vec4(pass_radius * N, 0));
//	if (pass_world_pos.y > 0) {
//		discard;
//	}
    actual_pos = actual_pos / actual_pos.w;
    gl_FragDepth = 0.5 + 0.5 * actual_pos.z;
    fragColor = vec4(pass_scalar, 0, 0, 1.0f);
    vec3 lightPos = (view * vec4(-1.7,1.7,1.7,1.0)).xyz;
    vec3 lightVec = normalize(lightPos - pass_pos.xyz);
    vec3 eye = normalize(-pass_pos.xyz);
    vec3 r = normalize(reflect(-lightVec, N));
    float d = max(dot(lightVec, N),0);
    float s = pow(max(dot(r,eye), 0.0f),20.0f);
    fragColor = 0.9 * d * fragColor + 0.1 * fragColor;
    //float srg = 0.2 * length(passVel.xyz);
    //fragmentColor += vec4(srg);
    fragColor.xyz += s;
    fragColor.w = 1;
}
