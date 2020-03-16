//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry/mesh.hpp"
#include <array>

namespace tostf
{
    struct Vertex {
        unsigned index;
        glm::vec4 pos;
    };

    class Tri_face_mesh {
    public:
        Tri_face_mesh() = default;
        explicit Tri_face_mesh(std::shared_ptr<Geometry> geom);
        std::shared_ptr<Mesh> to_mesh();
        std::vector<unsigned> get_indices();
        std::vector<std::array<unsigned, 3>> get_index_tuples() const;
        Mesh_stats calculate_mesh_stats();
        bool add_face(const std::array<unsigned, 3>& face);
        void add_faces(const std::vector<std::array<unsigned, 3>>& faces);
        void add_faces(const std::vector<unsigned>& faces);
        void remove_face(const std::array<unsigned, 3>& index_triple);
        void remove_faces(const std::vector<std::array<unsigned, 3>>& del_faces);
        bool has_face(const std::array<unsigned, 3>& index_triple);
        std::shared_ptr<Geometry> get_geometry() const;
        void set_geometry(const std::shared_ptr<Geometry>& geom);
        std::vector<std::array<unsigned, 3>> face_indices;
        std::vector<glm::vec4> face_normals;
        std::vector<glm::vec4> face_centers;
    private:
        std::shared_ptr<Geometry> _geometry;
    };
}
