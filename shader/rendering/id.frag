#version 450
flat in int pass_id;

layout (location = 0) out int id;

void main() {
    id = pass_id;
}
