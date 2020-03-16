#version 450

flat in vec4 pass_scalar;

out vec4 fragColor;

void main() {
    fragColor = vec4(pass_scalar.xyz, 1.0f);
}
