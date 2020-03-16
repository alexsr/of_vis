//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include <memory>
#include "geometry/mesh.hpp"
#include "halfedge_mesh.hpp"
#include "tri_face_mesh.hpp"

namespace tostf
{
    struct Structured_mesh {
        Structured_mesh() = default;
        explicit Structured_mesh(const std::shared_ptr<Mesh>& loaded_mesh);
        void init(const std::shared_ptr<Mesh>& loaded_mesh);
        void reset_tri_face_mesh();
        std::shared_ptr<Mesh> mesh;
        std::unique_ptr<Halfedge_mesh> halfedge;
        std::unique_ptr<Tri_face_mesh> tri_face_mesh;
        std::unique_ptr<Tri_face_mesh> original_tri_face_mesh;
    };
}
