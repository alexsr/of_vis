#version 450

in vec2 Frag_UV;
in vec4 Frag_Color;
out vec4 Out_Color;

uniform sampler2D tex;

void main() {
    Out_Color = Frag_Color * texture(tex, Frag_UV.st);
}
