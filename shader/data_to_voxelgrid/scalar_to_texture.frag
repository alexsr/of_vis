#version 450

flat in float pass_scalar;

out vec4 fragColor;

void main() {
    fragColor = vec4(pass_scalar, 0, 0, 1.0f);
}
