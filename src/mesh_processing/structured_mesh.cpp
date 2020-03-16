//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "structured_mesh.hpp"

tostf::Structured_mesh::Structured_mesh(const std::shared_ptr<Mesh>& loaded_mesh) {
    init(loaded_mesh);
}

void tostf::Structured_mesh::init(const std::shared_ptr<Mesh>& loaded_mesh) {
    mesh = loaded_mesh;
    halfedge = std::make_unique<Halfedge_mesh>(mesh);
    tri_face_mesh = std::make_unique<Tri_face_mesh>(mesh);
    original_tri_face_mesh = std::make_unique<Tri_face_mesh>(*tri_face_mesh);
}

void tostf::Structured_mesh::reset_tri_face_mesh() {
    tri_face_mesh = std::make_unique<Tri_face_mesh>(*original_tri_face_mesh);
}
