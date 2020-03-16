#version 450

//@inproceedings{baerentzen2008two,
//  title={Two methods for antialiased wireframe drawing with hidden line removal},
//  author={B{\ae}rentzen, J Andreas and Nielsen, Steen Lund and Gj{\o}l, Mikkel and Larsen, Bent D},
//  booktitle={Proceedings of the 24th Spring Conference on Computer Graphics},
//  pages={171--177},
//  year={2008},
//  organization={ACM}
//}

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 uv;

layout (std430, binding = 0) buffer selected_ssbo {
    float select_status[];
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec4 pass_position;
out vec3 pass_normal_gs;
out float selected;

void main() {
    selected = select_status[gl_VertexID];
    pass_normal_gs = vec3(transpose(inverse(view * model)) * normal);
    pass_position = proj * view * model * position;
    gl_Position = proj * view * model * position;
}
